# OrbitalPan

OrbitalPan is a stereo VST3 audio effect for Ableton Live and other VST3 hosts. Drag the orange point around the circular pad to place the signal from left to right and from front to back.

- Left/right uses equal-power stereo panning.
- Moving toward the front makes the signal slightly brighter and louder.
- Moving toward the back makes the signal darker, quieter, and more reverberant.
- **Depth** controls the strength of the front/back effect.
- **Mix** blends the original and processed signals.
- **Scope** shows the stereo image; switch between the processed **OUT** signal and original **IN** signal.

Current project details:

- Version: 0.1.0
- Format: 64-bit VST3 effect (stereo input and output)
- Language: C++17
- Framework: JUCE 8.0.12 for the tested Windows build; CMake also has a JUCE 7.0.9 fallback
- Windows: tested with Visual Studio 2022 and Ableton Live
- macOS: the instructions below target both Apple silicon and Intel Macs, but the target Mac still needs to be tested

> [!IMPORTANT]
> This repository contains source code, not a ready-to-run installer. A user following this guide will install build tools and compile the plug-in. For the simplest client handoff in the future, publish a signed and notarized universal macOS build containing both `arm64` and `x86_64` architectures.

## macOS: build and install for Ableton Live

These instructions are intentionally written for a non-technical user. Follow them in order. Do not open Ableton Live until the build is finished.

### 1. Open Terminal

In Finder, open **Applications → Utilities → Terminal**.

All commands below are pasted into Terminal. Press **Return** after each command.

### 2. Install Apple's command-line tools

```bash
xcode-select --install
```

Click **Install** in the window that appears and wait for it to finish. If Terminal says the tools are already installed, continue to the next step. The full Xcode application is not required.

### 3. Install Homebrew and CMake

Install [Homebrew](https://brew.sh/):

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

The Homebrew installer may show a **Next steps** section when it finishes. If it does, copy and run the commands shown there. Then close Terminal, reopen it, and run:

```bash
brew install cmake
```

Confirm that the required tools are available:

```bash
git --version
cmake --version
```

Both commands should print a version number.

### 4. Download OrbitalPan

```bash
mkdir -p "$HOME/Plugins"
cd "$HOME/Plugins"
git clone https://github.com/jwiley21/FirstPlugin.git
cd FirstPlugin
```

Download the same JUCE release used by the tested Windows build:

```bash
git clone --depth 1 --branch 8.0.12 https://github.com/juce-framework/JUCE.git "$HOME/JUCE"
```

### 5. Build and install the plug-in

Paste these commands one at a time:

```bash
cmake -S . -B build -DJUCE_DIR="$HOME/JUCE" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

The first command prepares the build. The second compiles OrbitalPan for the Mac being used and may take several minutes. On an Apple silicon Mac (M1, M2, M3, M4, or newer), a normally opened Terminal builds the native `arm64` plug-in required by native Ableton Live.

The project automatically installs the result here without requiring an administrator password:

```text
~/Library/Audio/Plug-Ins/VST3/OrbitalPan.vst3
```

Confirm that it exists:

```bash
test -d "$HOME/Library/Audio/Plug-Ins/VST3/OrbitalPan.vst3" && echo "OrbitalPan installed successfully"
```

If the success message appears, the build and installation worked.

### 6. Enable OrbitalPan in Ableton Live

1. Open Ableton Live.
2. Open **Live → Settings → Plug-Ins**. Older Live versions call this **Preferences**.
3. Turn **Use VST3 Plug-In System Folders** on.
4. Wait for Live's background plug-in scan to finish.
5. In Live's Browser, open **Plug-ins** and search for **OrbitalPan**.
6. Drag OrbitalPan onto a stereo audio track.

The user-level VST3 folder used above is one of Ableton's supported macOS locations. Do not select it as a custom VST3 folder; enabling the VST3 system folders is enough. See Ableton's official [macOS plug-in guide](https://help.ableton.com/hc/en-us/articles/209068929-Using-AU-and-VST-plug-ins-on-macOS).

### macOS troubleshooting

#### `brew: command not found`

Reopen Terminal and try again. If it still fails, rerun the Homebrew installer and carefully follow the **Next steps** it prints at the end.

#### `FirstPlugin` already exists

The project was downloaded previously. Update it instead of cloning it again:

```bash
cd "$HOME/Plugins/FirstPlugin"
git pull
```

Then repeat the two commands in step 5.

#### The build fails after a previous attempt

Configure into a fresh build folder, leaving the old one untouched:

```bash
cd "$HOME/Plugins/FirstPlugin"
cmake -S . -B build-mac -DJUCE_DIR="$HOME/JUCE" -DCMAKE_BUILD_TYPE=Release
cmake --build build-mac --config Release --parallel
```

#### OrbitalPan does not appear in Ableton Live

1. Quit every open copy of Live and reopen it.
2. Check that **Use VST3 Plug-In System Folders** is on.
3. Wait a few minutes for indexing to finish.
4. Hold **Option** while clicking **Rescan** to perform a deep rescan.
5. Confirm the plug-in folder exists with:

   ```bash
   ls "$HOME/Library/Audio/Plug-Ins/VST3/OrbitalPan.vst3"
   ```

On Apple silicon, Live 11.1 and newer only loads VST3 plug-ins built for Apple silicon when Live itself is running natively. This guide builds OrbitalPan natively on the Mac where the commands are run. See Ableton's [Apple silicon plug-in guidance](https://help.ableton.com/hc/en-us/articles/4410323149074-Plug-ins-on-Mac-in-Live-11-1-and-later).

If a command fails, copy the complete error text or take a screenshot before trying unrelated fixes.

## Windows: build and install

The Windows build has been tested with Visual Studio 2022 and Ableton Live.

### Requirements

- [Visual Studio 2022 Community](https://visualstudio.microsoft.com/vs/community/) with the **Desktop development with C++** workload
- [CMake](https://cmake.org/download/), selected in the Visual Studio Installer or installed separately
- Git, available from [Git for Windows](https://git-scm.com/download/win)

Open **Developer PowerShell for VS 2022** as Administrator, then run:

```powershell
New-Item -ItemType Directory -Force "$HOME\Plugins" | Out-Null
Set-Location "$HOME\Plugins"
git clone https://github.com/jwiley21/FirstPlugin.git
git clone --depth 1 --branch 8.0.12 https://github.com/juce-framework/JUCE.git "$HOME\JUCE"
Set-Location FirstPlugin
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 "-DJUCE_DIR=$HOME\JUCE"
cmake --build build --config Release --parallel
```

Running the developer shell as Administrator lets the build copy OrbitalPan to the standard 64-bit VST3 folder:

```text
C:\Program Files\Common Files\VST3\OrbitalPan.vst3
```

In Ableton Live, open **Settings/Preferences → Plug-Ins**, enable **Use VST3 Plug-In System Folders**, and rescan. Ableton documents the same standard folder in its [Windows VST guide](https://help.ableton.com/hc/en-us/articles/209071729-Using-VST-plug-ins-on-Windows).

## Developer notes

The macOS guide uses JUCE 8.0.12 because that is the version in the tested Windows build. Pass any existing JUCE checkout to CMake with:

```bash
cmake -S . -B build -DJUCE_DIR=/absolute/path/to/JUCE -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

The build artifact remains under:

```text
build/OrbitalPan_artefacts/Release/VST3/OrbitalPan.vst3
```

To build the optional standalone application, configure with `-DORBITALPAN_BUILD_STANDALONE=ON`. The default build produces only VST3.

If `JUCE_DIR` is omitted, CMake downloads the repository's older JUCE 7.0.9 fallback through `FetchContent`. That fallback has not been used for the documented Windows test and is not the recommended client build path.
