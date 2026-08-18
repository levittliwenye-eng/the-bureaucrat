#!/usr/bin/env bash

set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

required=(
    LICENSE
    ASSETS_LICENSE.md
    TRADEMARKS.md
    NOTICE.md
    THIRD_PARTY_NOTICES.md
    PRIVACY.md
    CONTRIBUTING.md
    SECURITY.md
    CHANGELOG.md
    LICENSES/CC-BY-SA-4.0.txt
    LICENSES/JUCE-8.0.11.md
    LICENSES/VST3-SDK-MIT.txt
)

for path in "${required[@]}"; do
    if [[ ! -s "$root/$path" ]]; then
        echo "Missing required open-source file: $path" >&2
        exit 1
    fi
done

grep -Fq "GNU AFFERO GENERAL PUBLIC LICENSE" "$root/LICENSE"
grep -Fq "Attribution-ShareAlike 4.0 International" \
    "$root/LICENSES/CC-BY-SA-4.0.txt"
grep -Fq 'THE_BUREAUCRAT_JUCE_TAG "8.0.11"' "$root/CMakeLists.txt"
grep -Fq "FetchContent_Declare(JUCE" "$root/CMakeLists.txt"

scan_paths=(
    "$root/CMakeLists.txt"
    "$root/README.md"
    "$root/ASSETS_LICENSE.md"
    "$root/TRADEMARKS.md"
    "$root/NOTICE.md"
    "$root/PRIVACY.md"
    "$root/CONTRIBUTING.md"
    "$root/SECURITY.md"
    "$root/CHANGELOG.md"
    "$root/RELEASE_NOTES.md"
    "$root/Source"
    "$root/Tests/DspEngineTests.cpp"
    "$root/Tests/PluginHostSmoke.cpp"
    "$root/Tests/UiRender.cpp"
)

private_path_pattern="/Users"'/[^/]+/'
sales_platform_one="Fast""Spring"
sales_platform_two="Gum""road"
retired_english="Soviet"" Saturation"
retired_chinese_one="官僚""主义"
retired_chinese_two="苏""式"
paid_package="paid"" package"
closed_source="closed"".source"
public_forbidden_pattern="${private_path_pattern}|${sales_platform_one}|${sales_platform_two}|${paid_package}|${closed_source}|${retired_english}|${retired_chinese_one}|${retired_chinese_two}"

if grep -RInE "$public_forbidden_pattern" \
    "${scan_paths[@]}"; then
    echo "Public source scope contains private paths, sales copy, or retired wording." >&2
    exit 1
fi

if [[ -e "$root/.open-source-distribution" ]]; then
    sales_release_doc="FAST""SPRING_RELEASE.md"
    forbidden=(
        EULA_DRAFT.md
        "$sales_release_doc"
        OWNER_RELEASE_INPUTS.md
        PRODUCT_COPY.md
        RELEASE_CHECKLIST.md
        RELEASE_COMPLIANCE.md
        release-owner.conf.example
        Marketing
        ui素材
        UI/v3/assets
        UI/v3/reference
    )

    for path in "${forbidden[@]}"; do
        if [[ -e "$root/$path" ]]; then
            echo "Private or development-only path leaked into public export: $path" >&2
            exit 1
        fi
    done
fi

echo "Open-source repository checks passed."
