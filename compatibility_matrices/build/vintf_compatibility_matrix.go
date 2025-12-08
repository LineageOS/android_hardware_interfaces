// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package vintf

import (
	"fmt"
	"io"
	"strings"

	"github.com/google/blueprint"
	"github.com/google/blueprint/proptools"

	"android/soong/android"
	"android/soong/kernel/configs"
	"android/soong/selinux"
)

type dependencyTag struct {
	blueprint.BaseDependencyTag
	name string
}

var (
	pctx = android.NewPackageContext("android/vintf")

	assembleVintfRule = pctx.AndroidStaticRule("assemble_vintf", blueprint.RuleParams{
		Command:     `${assembleVintfEnv} ${assembleVintfCmd} -i ${inputs} -o ${out} ${extraArgs}`,
		CommandDeps: []string{"${assembleVintfCmd}", "${AvbToolCmd}"},
		Description: "assemble_vintf -i ${inputs}",
	}, "inputs", "extraArgs", "assembleVintfEnv")

	xmllintXsd = pctx.AndroidStaticRule("xmllint-xsd", blueprint.RuleParams{
		Command:     `$XmlLintCmd --quiet --schema $xsd $in > /dev/null && touch -a $out`,
		CommandDeps: []string{"$XmlLintCmd"},
		Restat:      true,
	}, "xsd")

	kernelConfigTag  = dependencyTag{name: "kernel-config"}
	schemaTag        = dependencyTag{name: "matrix-schema"}
	schemaModuleName = "compatibility_matrix_schema"
)

const (
	relpath                  = "vintf"
	emptyManifest            = "hardware/interfaces/compatibility_matrices/manifest.empty.xml"
	compatibilityEmptyMatrix = "hardware/interfaces/compatibility_matrices/compatibility_matrix.empty.xml"
	deviceFcmType            = "device_fcm"
	productFcmType           = "product_fcm"
)

type vintfCompatibilityMatrixProperties struct {
	// set the name of the output
	Stem *string

	// list of source compatibility matrix XML files
	Srcs []string

	// list of kernel_config modules to be combined to final output
	Kernel_configs []string

	// Type of the FCM type, the allowed type are device_fcm and product_fcm and it should only be used under hardware/interfaces/compatibility_matrices
	Type *string
}

type vintfCompatibilityMatrixRule struct {
	android.ModuleBase
	properties vintfCompatibilityMatrixProperties

	genFile   android.WritablePath
	phonyOnly bool
}

func init() {
	pctx.HostBinToolVariable("assembleVintfCmd", "assemble_vintf")
	pctx.HostBinToolVariable("XmlLintCmd", "xmllint")
	pctx.HostBinToolVariable("AvbToolCmd", "avbtool")
	android.RegisterModuleType("vintf_compatibility_matrix", vintfCompatibilityMatrixFactory)
}

func vintfCompatibilityMatrixFactory() android.Module {
	g := &vintfCompatibilityMatrixRule{}
	g.AddProperties(&g.properties)
	android.InitAndroidArchModule(g, android.DeviceSupported, android.MultilibCommon)
	return g
}

var _ android.AndroidMkDataProvider = (*vintfCompatibilityMatrixRule)(nil)

func (g *vintfCompatibilityMatrixRule) DepsMutator(ctx android.BottomUpMutatorContext) {
	android.ExtractSourcesDeps(ctx, g.properties.Srcs)
	ctx.AddDependency(ctx.Module(), kernelConfigTag, g.properties.Kernel_configs...)
	ctx.AddDependency(ctx.Module(), schemaTag, schemaModuleName)
}

func (g *vintfCompatibilityMatrixRule) timestampFilePath(ctx android.ModuleContext, path android.Path) android.WritablePath {
	return android.GenPathWithExt(ctx, "vintf-xmllint", path, "ts")
}

func (g *vintfCompatibilityMatrixRule) generateValidateBuildAction(ctx android.ModuleContext, path android.Path, schema android.Path) android.Path {
	timestamp := g.timestampFilePath(ctx, path)
	ctx.Build(pctx, android.BuildParams{
		Rule:        xmllintXsd,
		Description: "xmllint-xsd",
		Input:       path,
		Output:      timestamp,
		Implicit:    schema,
		Args: map[string]string{
			"xsd": schema.String(),
		},
	})
	return timestamp
}

func (g *vintfCompatibilityMatrixRule) getSchema(ctx android.ModuleContext) android.OptionalPath {
	schemaModule := ctx.GetDirectDepProxyWithTag(schemaModuleName, schemaTag)
	sfp, ok := android.OtherModuleProvider(ctx, schemaModule, android.SourceFilesInfoProvider)

	if !ok {
		ctx.ModuleErrorf("Implicit dependency %q has no srcs", ctx.OtherModuleName(schemaModule))
		return android.OptionalPath{}
	}

	schemaSrcs := sfp.Srcs
	if len(schemaSrcs) != 1 {
		ctx.PropertyErrorf(`srcs of implicit dependency %q has length %d != 1`, ctx.OtherModuleName(schemaModule), len(schemaSrcs))
		return android.OptionalPath{}
	}
	return android.OptionalPathForPath(schemaSrcs[0])
}

func (g *vintfCompatibilityMatrixRule) GenerateAndroidBuildActions(ctx android.ModuleContext) {
	// Types attribute only allow `device_fcm` or `product_fcm` if set and only restricted it being used under
	// `hardware/interfaces/compatibility_matrices` to prevent accidental external usages.
	matrixType := proptools.String(g.properties.Type)
	if matrixType != "" {
		if matrixType != deviceFcmType && matrixType != productFcmType {
			panic(fmt.Errorf("The attribute 'type' value must be either 'device_fcm' or 'product_fcm' if set!"))
		}
		if !strings.HasPrefix(android.PathForModuleSrc(ctx).String(), "hardware/interfaces/compatibility_matrices") {
			panic(fmt.Errorf("Attribute type can only be set for module under `hardware/interfaces/compatibility_matrices`!"))
		}
		if (len(g.properties.Srcs) + len(g.properties.Kernel_configs)) > 0 {
			panic(fmt.Errorf("Attribute 'type' and 'srcs' or 'kernel_configs' should not set simultaneously! To update inputs for this rule, edit vintf_compatibility_matrix.go directly."))
		}
	}

	outputFilename := proptools.String(g.properties.Stem)
	if outputFilename == "" {
		outputFilename = g.Name()
	}

	schema := g.getSchema(ctx)
	if !schema.Valid() {
		return
	}

	var validations android.Paths
	inputPaths := android.PathsForModuleSrc(ctx, g.properties.Srcs)
	for _, srcPath := range inputPaths {
		validations = append(validations, g.generateValidateBuildAction(ctx, srcPath, schema.Path()))
	}

	// No need to validate matrices from kernel configs because they are generated by
	// assemble_vintf.
	ctx.VisitDirectDepsProxyWithTag(kernelConfigTag, func(m android.ModuleProxy) {
		if k, ok := android.OtherModuleProvider(ctx, m, configs.KernelConfigInfoProvider); ok {
			inputPaths = append(inputPaths, k.OutputPath)
		} else {
			ctx.PropertyErrorf("kernel_configs",
				"module %q is not a kernel_config", ctx.OtherModuleName(m))
		}
	})

	// For product_compatibility_matrix.xml the source is from the product configuration
	// DEVICE_PRODUCT_COMPATIBILITY_MATRIX_FILE.
	extraArgs := []string{}
	if matrixType == productFcmType {
		productMatrixs := android.PathsForSource(ctx, ctx.Config().DeviceProductCompatibilityMatrixFile())
		if len(productMatrixs) > 0 {
			inputPaths = append(inputPaths, productMatrixs...)
			extraArgs = append(extraArgs, "-c", android.PathForSource(ctx, emptyManifest).String())
		} else {
			// For product_fcm, if DEVICE_PRODUCT_COMPATIBILITY_MATRIX_FILE not set, treat it as a phony target without any output generated.
			g.phonyOnly = true
			return
		}
	}

	// For framework_compatibility_matrix.device.xml the source may come from the product configuration
	// DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE or use compatibilityEmptyMatrix if not set. We can't
	// use a phony target because we still need to install framework_compatibility_matrix.device.xml to
	// include sepolicy versions.
	frameworkRuleImplicits := []android.Path{}

	if matrixType == deviceFcmType {
		frameworkMatrixs := android.PathsForSource(ctx, ctx.Config().DeviceFrameworkCompatibilityMatrixFile())
		if len(frameworkMatrixs) > 0 {
			inputPaths = append(inputPaths, frameworkMatrixs...)

			// Generate BuildAction for generating the check manifest.
			emptyManifestPath := android.PathForSource(ctx, emptyManifest)
			genCheckManifest := android.PathForModuleGen(ctx, "manifest.check.xml")
			checkManifestInputs := []android.Path{emptyManifestPath}
			genCheckManifestEnvs := []string{
				"BOARD_SEPOLICY_VERS=" + ctx.DeviceConfig().BoardSepolicyVers(),
				"VINTF_IGNORE_TARGET_FCM_VERSION=true",
			}

			ctx.Build(pctx, android.BuildParams{
				Rule:        assembleVintfRule,
				Description: "Framework Check Manifest",
				Implicits:   checkManifestInputs,
				Output:      genCheckManifest,
				Args: map[string]string{
					"inputs":           android.PathForSource(ctx, emptyManifest).String(),
					"extraArgs":        "",
					"assembleVintfEnv": strings.Join(genCheckManifestEnvs, " "),
				},
			})

			frameworkRuleImplicits = append(frameworkRuleImplicits, genCheckManifest)
			extraArgs = append(extraArgs, "-c", genCheckManifest.String())
		} else {
			inputPaths = append(inputPaths, android.PathForSource(ctx, compatibilityEmptyMatrix))
		}
	}

	g.genFile = android.PathForModuleGen(ctx, outputFilename)
	frameworkRuleImplicits = append(frameworkRuleImplicits, inputPaths...)

	validations = append(validations, g.generateValidateBuildAction(ctx, g.genFile, schema.Path()))
	ctx.Build(pctx, android.BuildParams{
		Rule:        assembleVintfRule,
		Description: "Framework Compatibility Matrix",
		Implicits:   frameworkRuleImplicits,
		Output:      g.genFile,
		Validations: validations,
		Args: map[string]string{
			"inputs":           strings.Join(inputPaths.Strings(), ":"),
			"extraArgs":        strings.Join(extraArgs, " "),
			"assembleVintfEnv": g.getAssembleVintfEnv(ctx),
		},
	})

	ctx.InstallFile(android.PathForModuleInstall(ctx, "etc", relpath), outputFilename, g.genFile)
}

func (g *vintfCompatibilityMatrixRule) getAssembleVintfEnv(ctx android.ModuleContext) string {
	if proptools.String(g.properties.Type) == deviceFcmType {
		assembleVintfEnvs := []string{
			// POLICYVERS defined in system/sepolicy/build/soong/policy.go
			fmt.Sprintf("POLICYVERS=%d", selinux.PolicyVers),
			fmt.Sprintf("PLATFORM_SEPOLICY_VERSION=%s", ctx.DeviceConfig().PlatformSepolicyVersion()),
			fmt.Sprintf("PLATFORM_SEPOLICY_COMPAT_VERSIONS=\"%s\"", strings.Join(ctx.DeviceConfig().PlatformSepolicyCompatVersions(), " ")),
		}

		if ctx.Config().BoardAvbEnable() {
			assembleVintfEnvs = append(assembleVintfEnvs, fmt.Sprintf("FRAMEWORK_VBMETA_VERSION=\"$$(${AvbToolCmd} add_hashtree_footer --print_required_libavb_version %s)\"", strings.Join(ctx.Config().BoardAvbSystemAddHashtreeFooterArgs(), " ")))
		} else {
			assembleVintfEnvs = append(assembleVintfEnvs, "FRAMEWORK_VBMETA_VERSION=\"0.0\"")
		}

		return strings.Join(assembleVintfEnvs, " ")
	}

	return ""
}

func (g *vintfCompatibilityMatrixRule) AndroidMk() android.AndroidMkData {
	if g.phonyOnly {
		return android.AndroidMkData{
			Custom: func(w io.Writer, name, prefix, moduleDir string, data android.AndroidMkData) {
				fmt.Fprintln(w, "\ninclude $(CLEAR_VARS)", " # vintf.vintf_compatibility_matrix")
				fmt.Fprintln(w, "LOCAL_PATH :=", moduleDir)
				fmt.Fprintln(w, "LOCAL_MODULE :=", name)
				fmt.Fprintln(w, "include $(BUILD_PHONY_PACKAGE)")
			},
		}
	}

	return android.AndroidMkData{
		Class:      "ETC",
		OutputFile: android.OptionalPathForPath(g.genFile),
		Extra: []android.AndroidMkExtraFunc{
			func(w io.Writer, outputFile android.Path) {
				fmt.Fprintln(w, "LOCAL_MODULE_RELATIVE_PATH :=", relpath)
				if proptools.String(g.properties.Stem) != "" {
					fmt.Fprintln(w, "LOCAL_MODULE_STEM :=", proptools.String(g.properties.Stem))
				}
			},
		},
	}
}
