# OrbitalPan – Mac (Apple Silicon M4) Build & Install Guide

This guide is written for **non-developers**. Follow it **top to bottom**, copy/paste commands exactly, and you’ll be done.

---

## What you’re doing (simple explanation)

You are:

1. Installing Apple’s required tools
2. Downloading JUCE (the plugin framework)
3. Downloading the OrbitalPan plugin code from GitHub
4. Building the plugin
5. Copying the finished plugin into Ableton’s VST3 folder

You **do not** need to understand the code.

---

## Step 0 — Open Terminal

1. Open **Finder**
2. Go to **Applications → Utilities**
3. Open **Terminal**

You will paste commands into this window.

---

## Step 1 — Install Apple developer tools (required)

Paste this and press **Enter**:

```bash
xcode-select --install
```

• A popup will appear → click **Install**
• Wait until it finishes (5–15 minutes)

---

## Step 2 — Install Homebrew (installer)

Paste this entire line and press **Enter**:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

• When it finishes, it may print **2 lines** telling you to add Homebrew to PATH
• **Copy and paste those lines too** if it tells you to

---

## Step 3 — Install required tools (Git + CMake)

Paste and press **Enter**:

```bash
brew install git cmake
```

Wait for it to finish.

---

## Step 4 — Download JUCE (one-time setup)

JUCE must live in a specific folder.

Paste **each line** one at a time:

```bash
mkdir -p ~/JUCE
cd ~/JUCE
git clone https://github.com/juce-framework/JUCE.git
```

When done, JUCE should be located at:

```
/Users/YOURNAME/JUCE/JUCE
```

---

## Step 5 — Download the OrbitalPan plugin repo

Paste these commands:

```bash
mkdir -p ~/Plugins
cd ~/Plugins
git clone https://github.com/jwiley21/FirstPlugin.git
```

This creates:

```
~/Plugins/FirstPlugin/
```

---

## Step 6 — Build the plugin (Apple Silicon / M4)

### Go into the plugin folder

```bash
cd ~/Plugins/FirstPlugin
```

### Configure the build (this sets everything up)

```bash
cmake -S . -B build -G Xcode -DJUCE_DIR=~/JUCE/JUCE
```

Wait until it says configuration is done.

### Build the plugin (Release version)

```bash
cmake --build build --config Release
```

This can take a few minutes. Let it finish.

---

## Step 7 — Find the finished plugin (.vst3 file)

Paste this:

```bash
find build -name "*.vst3"
```

You should see something like:

```
build/OrbitalPan_artefacts/Release/VST3/OrbitalPan.vst3
```

That file is the plugin.

---

## Step 8 — Install the plugin into Ableton

### Make sure the VST3 folder exists

```bash
sudo mkdir -p /Library/Audio/Plug-Ins/VST3
```

### Copy the plugin there

```bash
sudo cp -R build/OrbitalPan_artefacts/Release/VST3/OrbitalPan.vst3 /Library/Audio/Plug-Ins/VST3/
```

• It will ask for your **Mac password** (typing is invisible — that’s normal)

---

## Step 9 — Open Ableton & rescan plugins

1. Open **Ableton Live**
2. Go to **Preferences → Plug-Ins**
3. Make sure **Use VST3 Plug-In System Folders** is **ON**
4. Click **Rescan** (or toggle VST3 off/on)

Then:
• Create an Audio Track
• Search for **OrbitalPan**
• Drop it on the track

---

## Step 10 — If Ableton blocks the plugin (common on macOS)

If Ableton doesn’t load it or macOS blocks it:

1. Open **System Settings → Privacy & Security**
2. Scroll down
3. If you see **OrbitalPan was blocked**, click **Allow Anyway**
4. Restart Ableton
5. Rescan plugins again

---

## Final Folder Check (for peace of mind)

• JUCE:

```
~/JUCE/JUCE
```

• Plugin repo:

```
~/Plugins/FirstPlugin
```

• Installed plugin:

```
/Library/Audio/Plug-Ins/VST3/OrbitalPan.vst3
```

---

## If anything fails

Stop and send **exact error text or a screenshot**.
Do NOT guess or retry random commands.

You’re done 🎛️
