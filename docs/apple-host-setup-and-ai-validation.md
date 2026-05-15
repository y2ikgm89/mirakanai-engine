# Apple ホストでの Metal / iOS 検証チェックリスト（MIRAIKANAI）

本手順は **Windows では実行不可** です。Apple ホスト（macOS）で実施してください。

## 1. 開発者ツール前提の確認

```bash
xcode-select -p
xcodebuild -version
xcrun -find xcodebuild
xcrun -find xcrun
```

- 期待:
  - `xcode-select` が Xcode 本体（`Xcode.app`）を指していること
  - `xcodebuild -version` が成功すること

## 2. Metal ツールの有無確認

```bash
xcrun --find metal
xcrun --find metallib
xcrun --find metallib ; echo $?
```

- 期待:
  - 両コマンドが有効なパスを返すこと

## 3. SDK とシミュレーター状態確認

```bash
xcrun --sdk iphonesimulator --show-sdk-version
xcrun --sdk iphoneos --show-sdk-version
xcrun simctl list runtimes
xcrun simctl list devices
```

- `check-apple-host-evidence.ps1` が `host=macos` で `not-configured` 以外へ進むことを目標にします。

## 4. MIRAIKANAI リポジトリ検証（Apple 側）

```bash
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireDirectCMake
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mobile-packaging.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-apple-host-evidence.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- `validate.ps1` は Windows 以前に実行したものと同じゲートを使うため、Apple 固有の
  - `metal` / `metallib`
  - `apple-host-evidence`
  - iOS シミュレーター関連
  の結果が変化します。

## 5. 結果記録（必須）

- 実行コマンドとログを `docs/` 側の該当運用ノート or 進行中の plan 記録に追記
- ブロッカーが残る場合は再実行条件を明示（例: Xcode バージョン固定、Simulator runtime追加、署名設定）
