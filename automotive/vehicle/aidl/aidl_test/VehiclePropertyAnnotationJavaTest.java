package android.hardware.automotive.vehicle;

import static com.google.common.truth.Truth.assertWithMessage;

import static org.junit.Assert.fail;

import androidx.test.filters.SmallTest;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.util.Map;

@RunWith(JUnit4.class)
public class VehiclePropertyAnnotationJavaTest {

    private boolean doesAnnotationMapContainsAllProps(Map<Integer, Integer> annotationMap) {
        for (Field field : VehicleProperty.class.getDeclaredFields()) {
            int modifiers = field.getModifiers();
            try {
                if (Modifier.isStatic(modifiers) && Modifier.isFinal(modifiers)
                        && Modifier.isPublic(modifiers) && field.getType().equals(int.class)) {
                    int propId = field.getInt(/* obj= */ null);
                    if (propId == VehicleProperty.INVALID) {
                        // Skip INVALID_PROP.
                        continue;
                    }
                    if (annotationMap.get(propId) == null) {
                        return false;
                    }
                }
            } catch (IllegalAccessException e) {
                throw new IllegalStateException(
                        "Cannot access a member for VehicleProperty.class", e);
            }
        }
        return true;
    }

    @Test
    @SmallTest
    public void testChangeMode() {
        assertWithMessage("Outdated annotation-generated AIDL files. Please run "
                + "generate_annotation_enums.py to update.")
                .that(doesAnnotationMapContainsAllProps(ChangeModeForVehicleProperty.values))
                .isTrue();
    }

    @Test
    @SmallTest
    public void testAccess() {
        assertWithMessage("Outdated annotation-generated AIDL files. Please run "
                + "generate_annotation_enums.py to update.")
                .that(doesAnnotationMapContainsAllProps(AccessForVehicleProperty.values))
                .isTrue();
    }
}