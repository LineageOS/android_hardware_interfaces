/*
 * Copyright 2020 The Android Open Source Project
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

package android.hardware.authsecret;

/**
 * This security HAL allows vendor components to be cryptographically tied to
 * the primary user's credential. For example, security hardware can require
 * proof that the credential is known before applying updates.
 *
 */
@VintfStability
interface IAuthSecret {
    /**
     * When the primary user is unlocked, this method is passed a secret to
     * prove that is has been successfully unlocked. The primary user can either
     * be unlocked by a person entering their credential or by another party
     * using an escrow token e.g. a device administrator.
     *
     * The first time this is called, the secret must be used to provision state
     * that depends on the primary user's secret. The same secret must be passed
     * on each call until the next factory reset.
     *
     * Upon factory reset, any dependence on the secret must be removed as that
     * secret is now lost and must never be derived again. A new secret must be
     * created for the new primary user which must be used to newly provision
     * state the first time this method is called after factory reset.
     *
     * The secret must be at least 16 bytes, or the secret must be dropped.
     *
     * @param secret blob derived from the primary user's credential.
     */
    oneway void setPrimaryUserCredential(in byte[] secret);
}
