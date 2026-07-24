# Legal and Asset Policy

## Purpose

Deep Shelter 3D is an original homebrew game for Nintendo 3DS. It may implement broad game-design ideas common to shelter-management games, but it must not copy protected expression, branding, source code, data or assets from Fallout Shelter or any other commercial title.

## Prohibited material

The repository must not contain:

- extracted, converted, traced or recreated game files from commercial products;
- proprietary source code, decompiled code or data tables;
- trademarks, logos, character names, faction names or distinctive UI copied from Fallout;
- copied dialogue, item descriptions, quests, story text or localization;
- music, sound effects, fonts, icons, textures, models or animations without a compatible license;
- material covered by an NDA or a commercial console SDK;
- private keys, signing keys or credentials.

## Accepted sources

Assets may be used only when they are:

1. created specifically for this project by a contributor;
2. public-domain or CC0;
3. under a permissive license compatible with redistribution in source and binary releases;
4. generated with tooling whose terms allow the intended use, with the generation process documented;
5. commissioned with written permission assigning the required rights to the project.

Assets requiring attribution must include the exact attribution text in `assets/ATTRIBUTION.md` and in the release documentation where required.

## Required manifest entry

Every committed non-code asset must be listed in `assets/manifest.csv` with:

- repository path;
- asset category;
- author or generator;
- source URL or `original`;
- license identifier;
- creation or acquisition date;
- notes proving provenance.

Generated release assets must identify their generator script rather than pretending to be hand-authored.

## AI-generated assets

AI-generated material is permitted only when:

- the tool and model are recorded;
- the prompt or a reproducible summary is retained;
- no copyrighted character, logo, screenshot or proprietary asset is supplied as a transformation target;
- the output is reviewed for accidental resemblance to protected branding;
- the applicable tool terms permit redistribution.

## Naming and visual identity

Use the original working identity `Deep Shelter 3D`. Do not use `Fallout`, `Vault-Tec`, `Pip-Boy`, recognizable franchise symbols or confusingly similar names in the game, metadata, package name, screenshots or marketing.

## Review requirements

Every pull request adding or changing an asset must answer:

- Who created it?
- Under which license may it be redistributed?
- Is its manifest entry complete?
- Does it resemble protected commercial material?
- Is attribution required?

The automated policy check is a minimum guardrail, not a substitute for human review.

## Incident response

If provenance is uncertain:

1. remove the asset from builds immediately;
2. replace it with a clearly marked original placeholder;
3. document the concern in an issue;
4. restore the asset only after provenance has been verified.
