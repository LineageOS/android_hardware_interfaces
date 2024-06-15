/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.car.tool;

import com.github.javaparser.StaticJavaParser;
import com.github.javaparser.ast.CompilationUnit;
import com.github.javaparser.ast.body.AnnotationDeclaration;
import com.github.javaparser.ast.body.FieldDeclaration;
import com.github.javaparser.ast.body.VariableDeclarator;
import com.github.javaparser.ast.comments.Comment;
import com.github.javaparser.ast.expr.AnnotationExpr;
import com.github.javaparser.ast.expr.ArrayInitializerExpr;
import com.github.javaparser.ast.expr.Expression;
import com.github.javaparser.ast.expr.NormalAnnotationExpr;
import com.github.javaparser.ast.expr.SingleMemberAnnotationExpr;
import com.github.javaparser.ast.expr.UnaryExpr;
import com.github.javaparser.ast.type.ClassOrInterfaceType;
import com.github.javaparser.javadoc.Javadoc;
import com.github.javaparser.javadoc.JavadocBlockTag;
import com.github.javaparser.javadoc.description.JavadocDescription;
import com.github.javaparser.javadoc.description.JavadocDescriptionElement;
import com.github.javaparser.javadoc.description.JavadocInlineTag;
import com.github.javaparser.resolution.declarations.ResolvedFieldDeclaration;
import com.github.javaparser.resolution.declarations.ResolvedReferenceTypeDeclaration;
import com.github.javaparser.symbolsolver.JavaSymbolSolver;
import com.github.javaparser.symbolsolver.javaparsermodel.declarations.JavaParserFieldDeclaration;
import com.github.javaparser.symbolsolver.model.resolution.TypeSolver;
import com.github.javaparser.symbolsolver.resolution.typesolvers.CombinedTypeSolver;
import com.github.javaparser.symbolsolver.resolution.typesolvers.JavaParserTypeSolver;
import com.github.javaparser.symbolsolver.resolution.typesolvers.ReflectionTypeSolver;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import org.json.JSONArray;
import org.json.JSONObject;

public final class EmuMetadataGenerator {
    private static final String DEFAULT_PACKAGE_NAME = "android.hardware.automotive.vehicle";
    private static final String INPUT_DIR_OPTION = "--input_dir";
    private static final String INPUT_FILES_OPTION = "--input_files";
    private static final String PACKAGE_NAME_OPTION = "--package_name";
    private static final String OUTPUT_JSON_OPTION = "--output_json";
    private static final String OUTPUT_EMPTY_FILE_OPTION = "--output_empty_file";
    private static final String CHECK_AGAINST_OPTION = "--check_against";
    private static final String USAGE = "EnumMetadataGenerator " + INPUT_DIR_OPTION
            + " [path_to_aidl_gen_dir] " + INPUT_FILES_OPTION + " [input_files] "
            + PACKAGE_NAME_OPTION + " [package_name] " + OUTPUT_JSON_OPTION + " [output_json] "
            + OUTPUT_EMPTY_FILE_OPTION + " [output_header_file] " + CHECK_AGAINST_OPTION
            + " [json_file_to_check_against]\n"
            + "Parses the VHAL property AIDL interface generated Java files to a json file to be"
            + " used by emulator\n"
            + "Options: \n" + INPUT_DIR_OPTION
            + ": the path to a directory containing AIDL interface Java files, "
            + "either this or input_files must be specified\n" + INPUT_FILES_OPTION
            + ": one or more Java files, this is used to decide the input "
            + "directory\n" + PACKAGE_NAME_OPTION
            + ": the optional package name for the interface, by default is " + DEFAULT_PACKAGE_NAME
            + "\n" + OUTPUT_JSON_OPTION + ": The output JSON file\n" + OUTPUT_EMPTY_FILE_OPTION
            + ": Only used for check_mode, this file will be created if "
            + "check  passed\n" + CHECK_AGAINST_OPTION
            + ": An optional JSON file to check against. If specified, the "
            + "generated output file will be checked against this file, if they are not the same, "
            + "the script will fail, otherwise, the output_empty_file will be created\n"
            + "For example: \n"
            + "EnumMetadataGenerator --input_dir out/soong/.intermediates/hardware/"
            + "interfaces/automotive/vehicle/aidl_property/android.hardware.automotive.vehicle."
            + "property-V3-java-source/gen/ --package_name android.hardware.automotive.vehicle "
            + "--output_json /tmp/android.hardware.automotive.vehicle-types-meta.json";
    private static final String VEHICLE_PROPERTY_FILE = "VehicleProperty.java";
    private static final String CHECK_FILE_PATH =
            "${ANDROID_BUILD_TOP}/hardware/interfaces/automotive/vehicle/aidl/emu_metadata/"
            + "android.hardware.automotive.vehicle-types-meta.json";

    // Emulator can display at least this many characters before cutting characters.
    private static final int MAX_PROPERTY_NAME_LENGTH = 30;

    /**
     * Parses the enum field declaration as an int value.
     */
    private static int parseIntEnumField(FieldDeclaration fieldDecl) {
        VariableDeclarator valueDecl = fieldDecl.getVariables().get(0);
        Expression expr = valueDecl.getInitializer().get();
        if (expr.isIntegerLiteralExpr()) {
            return expr.asIntegerLiteralExpr().asInt();
        }
        // For case like -123
        if (expr.isUnaryExpr() && expr.asUnaryExpr().getOperator() == UnaryExpr.Operator.MINUS) {
            return -expr.asUnaryExpr().getExpression().asIntegerLiteralExpr().asInt();
        }
        System.out.println("Unsupported expression: " + expr);
        System.exit(1);
        return 0;
    }

    private static boolean isPublicAndStatic(FieldDeclaration fieldDecl) {
        return fieldDecl.isPublic() && fieldDecl.isStatic();
    }

    private static String getFieldName(FieldDeclaration fieldDecl) {
        VariableDeclarator valueDecl = fieldDecl.getVariables().get(0);
        return valueDecl.getName().asString();
    }

    private static class Enum {
        Enum(String name, String packageName) {
            this.name = name;
            this.packageName = packageName;
        }

        public String name;
        public String packageName;
        public final List<ValueField> valueFields = new ArrayList<>();
    }

    private static class ValueField {
        public String name;
        public Integer value;
        public final List<String> dataEnums = new ArrayList<>();

        ValueField(String name, Integer value) {
            this.name = name;
            this.value = value;
        }
    }

    private static Enum parseEnumInterface(
            String inputDir, String dirName, String packageName, String enumName) throws Exception {
        Enum enumIntf = new Enum(enumName, packageName);
        CompilationUnit cu = StaticJavaParser.parse(new File(
                inputDir + File.separator + dirName + File.separator + enumName + ".java"));
        AnnotationDeclaration vehiclePropertyIdsClass =
                cu.getAnnotationDeclarationByName(enumName).get();

        List<FieldDeclaration> variables = vehiclePropertyIdsClass.findAll(FieldDeclaration.class);
        for (int i = 0; i < variables.size(); i++) {
            FieldDeclaration propertyDef = variables.get(i).asFieldDeclaration();
            if (!isPublicAndStatic(propertyDef)) {
                continue;
            }
            ValueField field =
                    new ValueField(getFieldName(propertyDef), parseIntEnumField(propertyDef));
            enumIntf.valueFields.add(field);
        }
        return enumIntf;
    }

    // A hacky way to make the key in-order in the JSON object.
    private static final class OrderedJSONObject extends JSONObject {
        OrderedJSONObject() {
            try {
                Field map = JSONObject.class.getDeclaredField("nameValuePairs");
                map.setAccessible(true);
                map.set(this, new LinkedHashMap<>());
                map.setAccessible(false);
            } catch (IllegalAccessException | NoSuchFieldException e) {
                throw new RuntimeException(e);
            }
        }
    }

    private static String readFileContent(String fileName) throws Exception {
        StringBuffer contentBuffer = new StringBuffer();
        int bufferSize = 1024;
        try (BufferedReader reader = new BufferedReader(new FileReader(fileName))) {
            char buffer[] = new char[bufferSize];
            while (true) {
                int read = reader.read(buffer, 0, bufferSize);
                if (read == -1) {
                    break;
                }
                contentBuffer.append(buffer, 0, read);
            }
        }
        return contentBuffer.toString();
    }

    private static final class Args {
        public final String inputDir;
        public final String pkgName;
        public final String pkgDir;
        public final String output;
        public final String checkFile;
        public final String outputEmptyFile;

        public Args(String[] args) throws IllegalArgumentException {
            Map<String, List<String>> valuesByKey = new LinkedHashMap<>();
            String key = null;
            for (int i = 0; i < args.length; i++) {
                String arg = args[i];
                if (arg.startsWith("--")) {
                    key = arg;
                    continue;
                }
                if (key == null) {
                    throw new IllegalArgumentException("Missing key for value: " + arg);
                }
                if (valuesByKey.get(key) == null) {
                    valuesByKey.put(key, new ArrayList<>());
                }
                valuesByKey.get(key).add(arg);
            }
            String pkgName;
            List<String> values = valuesByKey.get(PACKAGE_NAME_OPTION);
            if (values == null) {
                pkgName = DEFAULT_PACKAGE_NAME;
            } else {
                pkgName = values.get(0);
            }
            String pkgDir = pkgName.replace(".", File.separator);
            this.pkgName = pkgName;
            this.pkgDir = pkgDir;
            String inputDir;
            values = valuesByKey.get(INPUT_DIR_OPTION);
            if (values == null) {
                List<String> inputFiles = valuesByKey.get(INPUT_FILES_OPTION);
                if (inputFiles == null) {
                    throw new IllegalArgumentException("Either " + INPUT_DIR_OPTION + " or "
                            + INPUT_FILES_OPTION + " must be specified");
                }
                inputDir = new File(inputFiles.get(0)).getParent().replace(pkgDir, "");
            } else {
                inputDir = values.get(0);
            }
            this.inputDir = inputDir;
            values = valuesByKey.get(OUTPUT_JSON_OPTION);
            if (values == null) {
                throw new IllegalArgumentException(OUTPUT_JSON_OPTION + " must be specified");
            }
            this.output = values.get(0);
            values = valuesByKey.get(CHECK_AGAINST_OPTION);
            if (values != null) {
                this.checkFile = values.get(0);
            } else {
                this.checkFile = null;
            }
            values = valuesByKey.get(OUTPUT_EMPTY_FILE_OPTION);
            if (values != null) {
                this.outputEmptyFile = values.get(0);
            } else {
                this.outputEmptyFile = null;
            }
        }
    }

    /**
     * Main function.
     */
    public static void main(final String[] args) throws Exception {
        Args parsedArgs;
        try {
            parsedArgs = new Args(args);
        } catch (IllegalArgumentException e) {
            System.out.println("Invalid arguments: " + e.getMessage());
            System.out.println(USAGE);
            System.exit(1);
            // Never reach here.
            return;
        }

        TypeSolver typeSolver = new CombinedTypeSolver(
                new ReflectionTypeSolver(), new JavaParserTypeSolver(parsedArgs.inputDir));
        StaticJavaParser.getConfiguration().setSymbolResolver(new JavaSymbolSolver(typeSolver));

        Enum vehicleProperty = new Enum("VehicleProperty", parsedArgs.pkgName);
        CompilationUnit cu = StaticJavaParser.parse(new File(parsedArgs.inputDir + File.separator
                + parsedArgs.pkgDir + File.separator + VEHICLE_PROPERTY_FILE));
        AnnotationDeclaration vehiclePropertyIdsClass =
                cu.getAnnotationDeclarationByName("VehicleProperty").get();

        Set<String> dataEnumTypes = new HashSet<>();
        List<FieldDeclaration> variables = vehiclePropertyIdsClass.findAll(FieldDeclaration.class);
        for (int i = 0; i < variables.size(); i++) {
            FieldDeclaration propertyDef = variables.get(i).asFieldDeclaration();
            if (!isPublicAndStatic(propertyDef)) {
                continue;
            }
            String propertyName = getFieldName(propertyDef);
            if (propertyName.equals("INVALID")) {
                continue;
            }

            Optional<Comment> maybeComment = propertyDef.getComment();
            if (!maybeComment.isPresent()) {
                System.out.println("missing comment for property: " + propertyName);
                System.exit(1);
            }
            Javadoc doc = maybeComment.get().asJavadocComment().parse();

            int propertyId = parseIntEnumField(propertyDef);
            // We use the first paragraph as the property's name
            String propertyDescription = doc.getDescription().toText().split("\n\n")[0];
            String name = propertyDescription;
            if (propertyDescription.indexOf("\n") != -1
                    || propertyDescription.length() > MAX_PROPERTY_NAME_LENGTH) {
                // The description is too long, we just use the property name.
                name = propertyName;
            }
            ValueField field = new ValueField(name, propertyId);

            List<JavadocBlockTag> blockTags = doc.getBlockTags();
            List<Integer> dataEnums = new ArrayList<>();
            for (int j = 0; j < blockTags.size(); j++) {
                String commentTagName = blockTags.get(j).getTagName();
                String commentTagContent = blockTags.get(j).getContent().toText();
                if (!commentTagName.equals("data_enum")) {
                    continue;
                }
                field.dataEnums.add(commentTagContent);
                dataEnumTypes.add(commentTagContent);
            }

            vehicleProperty.valueFields.add(field);
        }

        List<Enum> enumTypes = new ArrayList<>();
        enumTypes.add(vehicleProperty);

        for (String dataEnumType : dataEnumTypes) {
            Enum dataEnum = parseEnumInterface(
                    parsedArgs.inputDir, parsedArgs.pkgDir, parsedArgs.pkgName, dataEnumType);
            enumTypes.add(dataEnum);
        }

        // Output enumTypes as JSON to output.
        JSONArray jsonEnums = new JSONArray();
        for (int i = 0; i < enumTypes.size(); i++) {
            Enum enumType = enumTypes.get(i);

            JSONObject jsonEnum = new OrderedJSONObject();
            jsonEnum.put("name", enumType.name);
            jsonEnum.put("package", enumType.packageName);
            JSONArray values = new JSONArray();
            jsonEnum.put("values", values);

            for (int j = 0; j < enumType.valueFields.size(); j++) {
                ValueField valueField = enumType.valueFields.get(j);
                JSONObject jsonValueField = new OrderedJSONObject();
                jsonValueField.put("name", valueField.name);
                jsonValueField.put("value", valueField.value);
                if (!valueField.dataEnums.isEmpty()) {
                    JSONArray jsonDataEnums = new JSONArray();
                    for (String dataEnum : valueField.dataEnums) {
                        jsonDataEnums.put(dataEnum);
                    }
                    jsonValueField.put("data_enums", jsonDataEnums);
                    // To be backward compatible with older format where data_enum is a single
                    // entry.
                    jsonValueField.put("data_enum", valueField.dataEnums.get(0));
                }
                values.put(jsonValueField);
            }

            jsonEnums.put(jsonEnum);
        }

        try (FileOutputStream outputStream = new FileOutputStream(parsedArgs.output)) {
            outputStream.write(jsonEnums.toString(4).getBytes());
        }
        System.out.println("Input at folder: " + parsedArgs.inputDir
                + " successfully parsed. Output at: " + parsedArgs.output);

        if (parsedArgs.checkFile != null) {
            String checkFileContent = readFileContent(parsedArgs.checkFile);
            String generatedFileContent = readFileContent(parsedArgs.output);
            String generatedFilePath = new File(parsedArgs.output).getAbsolutePath();
            if (!checkFileContent.equals(generatedFileContent)) {
                System.out.println("The file: " + CHECK_FILE_PATH + " needs to be updated, run: "
                        + "\n\ncp " + generatedFilePath + " " + CHECK_FILE_PATH + "\n");
                System.exit(1);
            }

            if (parsedArgs.outputEmptyFile != null) {
                try (FileOutputStream outputStream =
                                new FileOutputStream(parsedArgs.outputEmptyFile)) {
                    // Do nothing, just create the file.
                }
            }
        }
    }
}
