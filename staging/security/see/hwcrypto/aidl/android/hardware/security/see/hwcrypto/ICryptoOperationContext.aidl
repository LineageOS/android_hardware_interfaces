/*
 * Copyright 2024 The Android Open Source Project
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
package android.hardware.security.see.hwcrypto;

/*
 * Token that can be used to execute more commands when passed as an input on a
 * <code>CryptoOperationSet::context</code> parcelable. It represents an operation being executed
 * and is valid until a <code>CryptoOperation::Finish</code> is issued using the token. The
 * operation in progress context includes any memory buffer previously mapped by a
 * <code>CryptoOperation::SetMemoryBuffer</code> call.
 */
interface ICryptoOperationContext {}
