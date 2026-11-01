"""Rasterize UI type at its actual logical size. No generated artwork is modified."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import json
root=Path(__file__).resolve().parents[1]
atlas=Image.new('RGBA',(1024,1024))
draw=ImageDraw.Draw(atlas)
glyphs=[]
for family,(file,size,top) in enumerate([
    ('Rajdhani-Bold.ttf',16,0),('Bungee-Regular.ttf',17,96),
    ('Bungee-Regular.ttf',25,240),('Bungee-Regular.ttf',56,480)]):
    font=ImageFont.truetype(str(root/'assets/fonts'/file),size)
    cw,ch=(32,16) if family==0 else ((32,24) if family==1 else ((32,40) if family==2 else (64,88)))
    cols=1024//cw
    row=[]
    for code in range(32,127):
        char=chr(code);i=code-32;x=(i%cols)*cw;y=top+(i//cols)*ch
        bounds=font.getbbox(char,anchor='ls');w=bounds[2]-bounds[0];h=bounds[3]-bounds[1]
        draw.text((x+2-bounds[0],y+2-bounds[1]),char,font=font,fill='white',anchor='ls')
        row.append([x+2,y+2,w,h,round(font.getlength(char)),bounds[1]-font.getbbox('A',anchor='ls')[1]])
    glyphs.append(row)
atlas.save(root/'assets/ui-font-v15.png')
header='#pragma once\nnamespace dh {\nstruct UiGlyph{int x,y,w,h,advance,dy;};\ninline constexpr UiGlyph uiGlyphs[4][95]={\n'
for row in glyphs:header+='{'+','.join('{'+','.join(map(str,g))+'}' for g in row)+'},\n'
header+='};\n}\n'
(root/'src/ui_font_data.hpp').write_text(header,encoding='utf8')
print('UI glyph atlas: 380 glyphs at four native sizes')
