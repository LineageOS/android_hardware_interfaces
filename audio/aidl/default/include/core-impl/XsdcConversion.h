#include <string>
#include <unordered_map>
#include <unordered_set>

#include <aidl/android/media/audio/common/AudioHalCapCriterion.h>
#include <aidl/android/media/audio/common/AudioHalCapCriterionType.h>
#include <aidl/android/media/audio/common/AudioHalVolumeCurve.h>
#include <aidl/android/media/audio/common/AudioPort.h>
#include <android_audio_policy_configuration.h>
#include <android_audio_policy_configuration_enums.h>
#include <android_audio_policy_engine_configuration.h>
#include <media/AidlConversionUtil.h>

#include "core-impl/Module.h"

namespace aidl::android::hardware::audio::core::internal {

ConversionResult<::aidl::android::media::audio::common::AudioHalCapCriterion>
convertCapCriterionToAidl(
        const ::android::audio::policy::engine::configuration::CriterionType& xsdcCriterion);
ConversionResult<::aidl::android::media::audio::common::AudioHalCapCriterionType>
convertCapCriterionTypeToAidl(
        const ::android::audio::policy::engine::configuration::CriterionTypeType&
                xsdcCriterionType);
ConversionResult<::aidl::android::media::audio::common::AudioHalVolumeCurve::CurvePoint>
convertCurvePointToAidl(const std::string& xsdcCurvePoint);
ConversionResult<std::unique_ptr<Module::Configuration>> convertModuleConfigToAidl(
        const ::android::audio::policy::configuration::Modules::Module& moduleConfig);
}  // namespace aidl::android::hardware::audio::core::internal
