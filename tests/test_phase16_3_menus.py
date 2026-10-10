"""Static guardrails for the compact action-menu integration.

This checks command coverage and lifetime-safe asynchronous menu dispatch.
It does not substitute for compiling JUCE or physical UI tests.
"""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
menus = (ROOT / "app/JuceCompactActionMenus.hpp").read_text(encoding="utf-8")
main = (ROOT / "app/juce_main.cpp").read_text(encoding="utf-8")
for group in ("File", "Edit", "View", "Tools", "Help"):
    assert f"CompactMenuGroup::{group}" in menus
ids = {int(x) for x in re.findall(r"menu\.addItem\((\d+),", menus)}
routes = {int(x) for x in re.findall(r"case\s+(\d+)\s*:", main)}
assert ids <= routes, f"Menu IDs without routing: {ids-routes}"
assert len(ids) == 18
assert "showCompactMenu" in main
assert "juce::Component::SafePointer<MainComponent>" in main
assert "safeThis.getComponent()" in main
assert "showNewProjectMenu()" in main
assert "showCommandPalette()" in main
assert "showExportMenu()" in main
assert "setSize(1440,1040)" in main
print("PASS: compact menus have five groups, 18 routed IDs and SafePointer dispatch")
