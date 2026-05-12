// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

package dev.mirakanai.android;

import com.google.androidgamesdk.GameActivity;

public final class MirakanaiActivity extends GameActivity {
    static {
        System.loadLibrary("mirakanai_android");
    }
}
