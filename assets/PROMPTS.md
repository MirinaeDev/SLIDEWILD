# Assets de référence — essai 02

Génération/édition avec l’outil intégré imagegen. Source : assets/target-reference.png, fournie par l’utilisateur. Les trois premiers essais génériques ont été écartés et ne sont pas consommés par le jeu. Les sorties retenues sont des dérivés par édition générative, et non des extractions pixel pour pixel garanties.

Les fichiers PNG originaux sont conservés. Les .rgba sont une sérialisation sans redimensionnement ni retouche, via tools/pack_assets.py. Le moteur sélectionne les coordonnées de texture et l’échelle à l’affichage.


## rider-reference.png

Use case: background-extraction. EDIT TARGET: the attached image. Extract ONLY the existing longboard rider and his entire skateboard from the lower middle of this exact image. Preserve his exact pose, silhouette, proportions, white-and-black helmet, white grey shirt, black shorts, orange wheels, every original color and the existing coarse painterly pixel clusters. This is an extraction, NOT a redesign and NOT a new illustration. Remove absolutely all road, shadow cast onto road, scenery and HUD, replace with genuine transparency. Crop closely around the rider and board with small transparent margins. Do not add any detail, sharpening, glow, specular highlights, black outline, alternate poses or new texture. Do not change his helmet to red. Do not reinterpret pixel style. Preserve the original downsampled-looking blocky brush clusters and muted sunset lighting.

## pine-reference.png

Use case: background-extraction. EDIT TARGET provided alpine longboard screenshot. Extract one complete alpine fir tree based DIRECTLY on the tall tree at x approximately 570 y35 to500 in this exact image. One single whole tree, isolated transparent background, full trunk and entire crown visible, modest margin. Preserve the reference's coarse square pixel brush clusters, sparse irregular horizontal branches and dark near-black pine green with muted olive sunlit patches. Match exact art language of screenshot. Do not create dense realistic needles, do not add orange glow, do not add detailed noise, no Christmas-tree cone shape, no smooth 3D rendering. The style is broad matte color shapes and strongly stepped edges. No text, no scenery, no ground shadow. Complete the hidden lower trunk minimally in the same style.

## valley-reference.png

Use case: precise-object-edit. EDIT TARGET is the provided screenshot. Create the clean distant scenery background plate for this exact scene. Preserve the exact sky gradient, sun position, mountain silhouettes, lake, villages, colors, matte coarse pixel-brush style and atmospheric depth already present in the image. Remove the foreground road, rider, HUD, hay bales, roadside signs, guardrail, left cliff and all large foreground trees. Fill their former areas with an unobstructed continuation of the distant mountain valley, forests and lake using the existing distant scenery as the source. No foreground objects. Keep original wide 16:9 composition and the original skyline at exactly the same vertical position. The existing reference is the art direction: do NOT beautify, add detail, increase contrast, add glowing windows, change colors or make a new generic pixel art style. This is a background plate extraction and inpainting, not a reinterpretation. No text, no road, no rider.
