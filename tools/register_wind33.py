from pathlib import Path
from PIL import Image
import numpy as np
root=Path(__file__).resolve().parents[1];rows=[]
for name in ['sol','rio','nox','mei']:
 a=np.array(Image.open(root/f'assets/wind-{name}-v32.png'));points=[]
 for k in range(6):
  x=k%3*512;y=k//3*512;mask=a[y:y+512,x:x+512,3]>178;yy,xx=np.where(mask);top=yy.min();cap=np.where(mask & (np.indices(mask.shape)[0]<top+55));points.append((x+float(np.mean(cap[1])),y+top))
 vals=[]
 for k in range(6):
  base=points[k//3*3];vals.append([round(points[k][0]-base[0])/1536,round(points[k][1]-base[1])/1024])
 rows.append(vals)
out='#pragma once\nnamespace dh {\ninline constexpr float windRegistration[4][6][2]={\n'+',\n'.join('{'+','.join('{'+','.join(f'{v:.8f}f' for v in p)+'}' for p in row)+'}' for row in rows)+'\n};\n}\n'
(root/'src/wind_registration.hpp').write_text(out)
print(out)
