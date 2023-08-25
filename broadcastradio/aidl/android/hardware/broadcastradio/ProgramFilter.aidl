/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.broadcastradio;

import android.hardware.broadcastradio.IdentifierType;
import android.hardware.broadcastradio.ProgramIdentifier;

/**
 * Large-grain filter to the program list.
 *
 * This is meant to reduce binder transaction bandwidth, not for fine-grained
 * filtering user might expect.
 *
 * The filter is designed as conjunctive normal form: the entry that passes the
 * filter must satisfy all the clauses (members of this struct). Vector clauses
 * are disjunctions of literals. In other words, there is AND between each
 * high-level group and OR inside it.
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable ProgramFilter {
    /**
     * List of identifier types that are filtered by the filter.
     *
     * If the program list entry contains at least one identifier of the type
     * listed, it satisfies this condition.
     *
     * Empty list means no filtering on identifier type.
     */
    IdentifierType[] identifierTypes;

    /**
     * List of identifiers that are filtered by the filter.
     *
     * If the program list entry contains at least one listed identifier,
     * it satisfies this condition.
     *
     * Empty list means no filtering on identifier.
     */
    ProgramIdentifier[] identifiers;

    /**
     * Includes non-tunable entries that define tree structure on the
     * program list (i.e. DAB ensembles).
     */
    boolean includeCategories;

    /**
     * Disables updates on entry modifications.
     *
     * If {@code true}, 'modified' vector of {@link ProgramListChunk} must contain
     * list additions only. Once the program is added to the list, it's not
     * updated anymore.
     */
    boolean excludeModifications;
}
