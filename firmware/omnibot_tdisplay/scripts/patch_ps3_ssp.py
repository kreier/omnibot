from pathlib import Path

Import("env")


def patch_ps3_controller():
    libdeps_dir = Path(env.subst("$PROJECT_LIBDEPS_DIR")) / env.subst("$PIOENV")
    target = libdeps_dir / "Fork of PS3 Controller Host" / "src" / "Ps3Controller.cpp"
    if not target.exists():
        print(f"[patch_ps3_ssp] Warning: {target} not found yet")
        return

    text = target.read_text(encoding="utf-8")
    if "cfg.ssp_en = false;" in text and "esp_bluedroid_init_with_cfg(&cfg)" in text:
        return

    old = "        if (esp_bluedroid_init()) {"
    new = (
        "        esp_bluedroid_config_t cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();\n"
        "        cfg.ssp_en = false;\n"
        "        if (esp_bluedroid_init_with_cfg(&cfg)) {"
    )
    if old not in text:
        raise RuntimeError(
            f"[patch_ps3_ssp] Expected pattern not found in {target}; cannot apply SSP disable patch."
        )

    target.write_text(text.replace(old, new, 1), encoding="utf-8")
    print(f"[patch_ps3_ssp] Patched {target} (disabled Bluedroid SSP for DS3 clones)")


patch_ps3_controller()
