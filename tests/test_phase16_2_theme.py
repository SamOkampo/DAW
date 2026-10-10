"""Source-level guardrails for Phase 16.2 premium JUCE theme.

Tests are deliberately independent of C++ assert/NDEBUG and do not
claim installed-app accessibility or physical Windows QA.
"""

from pathlib import Path
import math
import re

ROOT = Path(__file__).resolve().parents[1]
THEME = (ROOT / "app/JuceTheme.hpp").read_text(encoding="utf-8")
SURFACES = [
    "app/juce_main.cpp",
    "app/JuceEditingSurface.hpp",
    "app/JuceSampleBrowser.hpp",
    "app/JuceStepSequencerSurface.hpp",
    "app/JucePianoSamplerSurface.hpp",
    "app/JuceAutomationAssistSurface.hpp",
]
COLORS = {
    "canvasTop": "101113", "surface": "181a1d",
    "surfaceRaised": "22252a", "surfaceHover": "2d3137",
    "borderSubtle": "363a40", "textPrimary": "ecece8",
    "textSecondary": "b6b5ae", "textMuted": "92918b",
    "accent": "c2783d", "accentDeep": "7d4e2c",
    "accentHot": "e09755", "focus": "e9b179",
    "success": "76ad76", "danger": "be6660",
}

def luminance(rgb):
    parts = [int(rgb[i:i+2], 16) / 255 for i in (0, 2, 4)]
    linear = [v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4 for v in parts]
    return sum(a * b for a, b in zip(linear, (0.2126, 0.7152, 0.0722)))

def contrast(a, b):
    bright, dark = sorted((luminance(a), luminance(b)), reverse=True)
    return (bright + 0.05) / (dark + 0.05)

def check():
    for token, hex_value in COLORS.items():
        pattern = rf"static\s+juce::Colour\s+{re.escape(token)}\(\)\s*\{{return\s+juce::Colour\(0xff{hex_value}\);\}}"
        assert re.search(pattern, THEME, re.I), f"Theme semantic token missing or changed: {token}"

    for name in ("textPrimary", "textSecondary", "textMuted"):
        value = contrast(COLORS[name], COLORS["surface"])
        assert value >= 4.5, f"{name} on dark surface has poor contrast: {value:.2f}:1"
    assert contrast(COLORS["textPrimary"], COLORS["accentDeep"]) >= 4.5
    assert contrast(COLORS["focus"], COLORS["surface"]) >= 4.5
    assert contrast(COLORS["textPrimary"], COLORS["accent"]) < 4.5, ("Re-evaluate the active background if the amber palette changes")

    legacy = ("0xff8f4cff", "0xff52d6c7", "0xffe45b98", "0xff7868e6", "0xffd94d8b")
    for path in SURFACES:
        text = (ROOT / path).read_text(encoding="utf-8").lower()
        assert not any(old in text for old in legacy), f"Legacy neon hex color in {path}"
    editing = (ROOT / "app/JuceEditingSurface.hpp").read_text(encoding="utf-8")
    assert "FlowTheme::clipPattern()" in editing, "Pattern needs distinct semantic color"
    assert "FlowTheme::meterSafe()" in editing, "Meter should use safe semantic color"
    assert "FlowTheme::focus()" in editing, "Playhead/focus should remain visible"
    assert "\\n    static" not in THEME, "Escaped newline inserted into C++ theme source"
    print("PASS: premium graphite/amber semantic theme, contrasts and key surfaces")

if __name__ == "__main__":
    check()
