# Xiaoke campus illustration prompt

Created: 2026-10-21. Tool: built-in imagegen. Intent: generate a new illustration using character references, not edit or copy a finished scene. Design direction and integration: Codex.

Reference 1: white/navy campus jacket portrait, school new-media article (2021-03-25), research ID `newmedia-13`.

Reference 2: official 4 × 4 chibi expression overview from the same article, research ID `newmedia-20`.

Source inventory and rights: [华小科素材与背景设计](../../../../docs/frontend/华小科素材与背景设计.md).

Final generation prompt:

```text
Use case: illustration-story.
Asset type: original landscape illustration asset for the existing HustMCL warm-paper desktop launcher background, NOT a UI mockup.
Input images: image 1 is the character identity reference (pink-haired Hua Xiaoke in white/navy campus jacket); image 2 is the official chibi expression reference sheet. Preserve the recognizable identity; make a new pose, scene and drawing.
Primary request: A polished illustrated campus postcard, landscape 1536x1024. A single charming chibi Hua Xiaoke sits on a low warm stone wall at the RIGHT HALF of the image, under a delicate green tree canopy, holding an open navy notebook on her lap, one hand giving a small friendly wave. Full chibi figure and feet visible, 2.7 heads tall. She has dusty rose pink hair in short low twin tails, curved little ahoge on top, pink eyes, small blue round hair clip with a simple white abstract glint (no exact seal or lettering), blue ribbon accents. White hooded campus jacket, navy trim, pale blue panels, navy tie, modest navy skirt and sneakers. Friendly calm expression, no exaggerated sparkles. Accuracy of pink twin tails/blue accents important.
Scene: abstract forest campus in warm late afternoon, restrained sage foliage and a few light ochre leaves, distant small cream academic buildings suggested by spare ink lines at far left. Soft pale warm-paper sky, spacious. A few simple square stepping stones subtly suggest a block-building game without any Minecraft logo or characters.
Composition: character fills rightmost 45% of canvas, approximately x=840..1380 and y=200..900. Left half mostly empty pale paper with delicate pale background architecture/foliage only, no lettering. Bottom wall ties the composition together. Keep full figure away from edges, generous breathable margins.
Style: clean hand-drawn editorial anime chibi illustration, warm precise charcoal fine line work, flat muted pastel color blocks with subtle watercolor shading, elegant contemporary Japanese campus picture-book sensibility, no glossy 3D. Overall cream #F4F0E7, pale jade #CCD8C4, dusty rose #D98D9B, accents slate blue #4C647B and deep red #8C1D2F used sparingly. Character should stand out gently but not be washed out. Beautiful restrained finished artwork.
Constraints: create only the illustration, no UI controls, no text, no numbers, no watermark, no exact university logo, no extra characters, no night scene, no neon, no heavy paper noise. Opaque background matching warm paper.
```

Saved delivery: `xiaoke-campus.jpg`, 1536 × 1024, JPEG quality 90, no chroma subsampling. The encoding step preserves the composition; the layout crops it through CSS / SVG instead of creating page-specific bitmap copies.
