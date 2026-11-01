// Authored scenery distribution for the additional courses. All placements
// use the actual route/heightfield and are rebuilt when changing destination.
struct BiomeProp {V3 p;float height;int kind;float heading=0;bool fixed=false;};
static std::vector<BiomeProp> biomeInstances;
static const SpriteContact& biomeContact(int biome,int kind){
 static const auto contacts=[](){
   std::array<std::array<SpriteContact,24>,4> result{};
   const char* names[4][4]={{"desert-props-v18","desert-details-v18","desert-habitat-v18","desert-habitat-v18"},
     {"urban-props-v18","urban-details-v18","urban-habitat-v18","urban-habitat-v18"},
     {"scotland-props-v18","scotland-details-v18","scotland-habitat-v18","scotland-understory-v19"},
     {"tropical-props-v18","tropical-details-v18","tropical-habitat-v18","tropical-understory-v19"}};
   for(int b=0;b<4;b++)for(int k=0;k<(b>=2?24:18);k++)result[b][k]=spriteContact(names[b][k/6],k>=18?understoryRects[b-2][k-18]:biomeProps[b][k]);
   return result;
 }();return contacts[biome-1][kind];
}
static bool cityPlaza(float s){for(float site:{660.f,1190.f,2080.f,3010.f})if(std::abs(s-site)<44)return true;return false;}
static float cityTerraceWidth(float s){float w=11;for(float site:{660.f,1190.f,2080.f,3010.f})w=std::max(w,11+11*clamp((60-std::abs(s-site))/16,0,1));return w;}
static void biomeBuilding(Mesh&m,const Road&road,float s,float offset,float width,float depth,float height,int tile){
 auto rr=road.at(s);V3 f{std::sin(rr.heading),0,std::cos(rr.heading)},q{f.z,0,-f.x},root=road.surface(s,offset);
 root.y=terrainField.height(root);V3 corners[4]={root-q*width*.5f-f*depth*.5f,root+q*width*.5f-f*depth*.5f,root+q*width*.5f+f*depth*.5f,root-q*width*.5f+f*depth*.5f};
 for(auto p:corners)if(road.locate(p).distance<9)return;
 float u=(tile%3)/3.f,v=(tile/3)/2.f;
 for(int j=0;j<4;j++){V3 a=corners[j],b=corners[(j+1)%4];artQuad(m,a+V3{0,height,0},b+V3{0,height,0},b,a,30,u+.002f,v+.003f,u+.331f,v+.497f);}
 quad(m,corners[0]+V3{0,height,0},corners[1]+V3{0,height,0},corners[2]+V3{0,height,0},corners[3]+V3{0,height,0},{.38f,.24f,.20f},2);
 box(m,root+V3{0,height+.13f,0},{width+.6f,.26f,depth+.6f},{.58f,.33f,.22f},rr.heading,2);
}
static void buildBiomeArchitecture(Mesh&m,const Road&road){
 biomeInstances.clear();
 // The stopping area has a real, raised backdrop beyond the last road sample.
 // It is visible through the finish arch and stays behind the runout corridor.
 auto end=road.at(road.size());V3 ef{std::sin(end.heading),0,std::cos(end.heading)},eq{ef.z,0,-ef.x};
 if(road.biome()==2){
   for(int side:{-1,1})for(int j=0;j<6;j++)
     biomeBuilding(m,road,4025+j*25.f,side*35.f,22,16,20+hash(float(j+side*9))*8,(j+(side>0?2:0))%6);
   for(int row=0;row<2;row++)for(int j=0;j<17;j++){
     float x=-136+j*17.f,z=20+row*23.f+hash(float(j+row*41))*5;
     V3 base=woodlandGround(road,z,x);float width=17.8f,height=14+hash(float(j*13+row))*15,depth=16;
     V3 v[4]={base-eq*width*.5f-ef*depth*.5f,base+eq*width*.5f-ef*depth*.5f,base+eq*width*.5f+ef*depth*.5f,base-eq*width*.5f+ef*depth*.5f};
     int tile=(j+row*2)%6;float u=(tile%3)/3.f,t=(tile/3)/2.f;
     for(int k=0;k<4;k++)artQuad(m,v[k]+V3{0,height,0},v[(k+1)%4]+V3{0,height,0},v[(k+1)%4],v[k],30,u+.002f,t+.003f,u+.331f,t+.497f);
     quad(m,v[0]+V3{0,height,0},v[1]+V3{0,height,0},v[2]+V3{0,height,0},v[3]+V3{0,height,0},{.23f,.20f,.24f},2);
     box(m,base+V3{0,height+.45f,0},{width+.4f,.9f,depth+.4f},{.40f,.34f,.30f},end.heading,2);
   }
 }
 if(road.biome()==1){
   for(int row=0;row<2;row++)for(int j=0;j<14;j++){
     V3 base=woodlandGround(road,20+row*30.f,-143+j*22.f);base.y-=3;
     float height=19+hash(float(j+row*31))*20;constexpr int n=10;V3 rings[4][n];
     for(int l=0;l<4;l++)for(int k=0;k<n;k++){
       float a=k*2*pi/n,rad=(15+hash(float(j*17+k))*4)*(1-l*.13f);
       rings[l][k]=base+V3{std::cos(a)*rad,height*l/3,std::sin(a)*rad};
     }
     for(int l=0;l<3;l++)for(int k=0;k<n;k++)quad(m,rings[l][k],rings[l][(k+1)%n],rings[l+1][(k+1)%n],rings[l+1][k],{1,1,1},18);
     for(int k=0;k<n;k++){triangle(m,base,rings[0][(k+1)%n],rings[0][k],{1,1,1},18);triangle(m,base+V3{0,height,0},rings[3][k],rings[3][(k+1)%n],{1,1,1},18);}
   }
 }
 if(road.biome()==1||road.biome()>=3){
   for(int j=0;j<180;j++){
     float at=28+j*22.f,seed=float(j+903);bool near=j%3!=0;
     if(near&&(std::abs(at-760)<55||std::abs(at-2020)<55))continue;
     float radius=near?2.8f+hash(seed)*2.3f:12+hash(seed)*12;
     float d=(j%3==0?1.f:-1.f)*(near?13+hash(seed+9)*8:46+hash(seed+9)*58);
     if(road.biome()>=3){radius*=.65f;d=std::copysign(std::abs(d)+3,d);}
     V3 root=road.surface(at,d);auto loc=road.locate(root);
     if(loc.distance<radius+8||at>3950)continue;
     float base=terrainField.height(root)-2;
     for(int k=0;k<8;k++)base=std::min(base,terrainField.height(root+V3{std::cos(k*pi/4)*radius,0,std::sin(k*pi/4)*radius})-1);
     root.y=base;float height=near?7+hash(seed+12)*10:22+hash(seed+12)*30;
     if(road.biome()>=3)height*=.45f;
     constexpr int sides=10,levels=6;V3 rings[levels][sides];
     const float ys[levels]={0,.16f,.31f,.66f,.87f,1},rs[levels]={1,.93f,.98f,.65f,.70f,.46f};
     for(int l=0;l<levels;l++)for(int k=0;k<sides;k++){
       float a=k*2*pi/sides+hash(seed+1)*3,rough=.82f+hash(seed+k*3)*.16f;
       rings[l][k]=root+V3{std::cos(a)*radius*rs[l]*rough,height*ys[l]+(l==0?0:hash(seed+k*3)*1.4f),std::sin(a)*radius*rs[l]*rough};
     }
     for(int l=0;l<levels-1;l++)for(int k=0;k<sides;k++){
       int n=(k+1)%sides;quad(m,rings[l][k],rings[l][n],rings[l+1][n],rings[l+1][k],{1,1,1},18);
     }
     V3 top=root+V3{0,height+.7f,0};
     for(int k=0;k<sides;k++){int n=(k+1)%sides;triangle(m,root,rings[0][n],rings[0][k],{1,1,1},18);triangle(m,top,rings[levels-1][k],rings[levels-1][n],{1,1,1},18);}
   }
   if(road.biome()==1)for(float at:{760.f,2020.f,4060.f}){
     biomeBuilding(m,road,at,-25,16,11,5.2f,at<1000?0:3);
     auto rr=road.at(at);
     // Timber shelter, fuel stop and an enclosed water tank make each relay
     // legible from the road without replacing the open desert by a forest.
     for(float z:{-5.f,5.f})for(float d:{-20.f,-12.f}){
       V3 p=road.surface(at+z,d);p.y=terrainField.height(p);
       box(m,p+V3{0,1.7f,0},{.20f,3.4f,.20f},{.43f,.27f,.17f},rr.heading,2);
     }
     V3 roof=road.surface(at,-16);roof.y=terrainField.height(roof)+3.5f;
     box(m,roof,{9,.22f,11.5f},{.61f,.31f,.20f},rr.heading,2);
     V3 tank=road.surface(at+18,-25);tank.y=terrainField.height(tank)+3;
     box(m,tank,{3.7f,6,3.7f},{.31f,.44f,.41f},rr.heading,15);
   }
 }else if(road.biome()==2){
   for(int j=0;j<299;j++)for(int side:{-1,1}){
     float at=22+j*13.2f;if(at>3960)continue;
     // Open terraces and harbor outlooks break up the street walls.
     if((side>0&&std::fmod(at,780.f)>570)||(side<0&&j%17==0))continue;
     auto rr=road.at(at);V3 f{std::sin(rr.heading),0,std::cos(rr.heading)},q{f.z,0,-f.x};
     float half=5.1f+hash(float(j+44))*.9f,deep=8+hash(float(j+side*57))*5;
     V3 front=road.surface(at,side*(cityPlaza(at)?23.f:11.1f)),back=front+q*(side*deep);
     V3 a=front-f*half,b=front+f*half,c=back+f*half,d=back-f*half;
     bool clear=true;for(auto p:{a,b,c,d,back})if(road.locate(p).distance<8.2f)clear=false;
     if(!clear)continue;
     float base=rr.p.y-.25f,height=9.2f+hash(float(j*3+side*23))*8;
     float bottom=base-2;for(auto p:{a,b,c,d})bottom=std::min(bottom,terrainField.height(p)-.6f);
     for(auto* p:{&a,&b,&c,&d})p->y=base;
     V3 corners[4]={a,b,c,d};int style=(j+(side>0?2:0))%6;
     for(int k=0;k<4;k++){
       V3 x=corners[k],y=corners[(k+1)%4];
       V3 lowX=x,lowY=y;lowX.y=lowY.y=bottom;
       artQuad(m,x,y,lowY,lowX,16,at*.1f,0,at*.1f+1,1,{.50f,.50f,.55f});
       int tile=k==0?style:(style+1)%6;float u=(tile%3)/3.f,v=(tile/3)/2.f;
       artQuad(m,x+V3{0,height,0},y+V3{0,height,0},y,x,30,u+.002f,v+.003f,u+1.f/3-.002f,v+.5f-.003f);
     }
     quad(m,a+V3{0,height,0},b+V3{0,height,0},c+V3{0,height,0},d+V3{0,height,0},{.20f,.16f,.19f},2);
     // Roof edging, chimneys and an occasional billboard-sized awning add
     // depth at the silhouette rather than relying on a flat facade sprite.
     box(m,(a+b)*.5f+V3{0,height,.0f},{.23f,.24f,half*2+.25f},{.48f,.43f,.40f},rr.heading,2);
     if(j%3==0)box(m,(a+c)*.5f+V3{0,height+.7f,0},{.9f,1.4f,.8f},{.30f,.25f,.27f},rr.heading,2);
     if(j%2==0){V3 p=front-q*(side*.65f)+V3{0,2.7f,0};box(m,p,{1.3f,.15f,half*1.3f},j%4?V3{.55f,.21f,.19f}:V3{.22f,.43f,.43f},rr.heading,2);}
     if(j%4==0)for(float floor:{4.5f,7.8f}){
       V3 p=front-q*(side*.7f)+V3{0,floor,0};
       box(m,p,{1.4f,.18f,half*1.4f},{.30f,.32f,.35f},rr.heading,2);
       box(m,p-q*(side*.6f)+V3{0,.8f,0},{.07f,.08f,half*1.4f},{.12f,.14f,.20f},rr.heading,15);
       for(int bar=-3;bar<=3;bar++)box(m,p-q*(side*.6f)+f*(bar*half*.21f)+V3{0,.42f,0},{.06f,.85f,.06f},{.12f,.14f,.20f},rr.heading,15);
     }
   }
   // Harbor warehouses, ribbed freight containers and skeletal dock cranes.
   for(int j=0;j<10;j++){
     float at=3000+j*112.f;auto rr=road.at(at);V3 root=road.surface(at,36);root.y=terrainField.height(root);
     for(int row=0;row<2;row++){
       V3 p=root+V3{std::cos(rr.heading)*row*6,1.5f,-std::sin(rr.heading)*row*6};
       V3 color=j%2?V3{.38f,.23f,.20f}:V3{.15f,.35f,.38f};
       box(m,p,{4.6f,3,10},color,rr.heading,2);
       for(int rib=0;rib<15;rib++){
         V3 along{std::sin(rr.heading)*(rib*.65f-4.5f),0,std::cos(rr.heading)*(rib*.65f-4.5f)};
         box(m,p+along,{4.7f,3.02f,.055f},color*1.3f,rr.heading,15);
       }
     }
     if(j%3==0){
       V3 p=root+V3{std::cos(rr.heading)*13,0,-std::sin(rr.heading)*13};
       for(float d:{-3.f,3.f})box(m,p+V3{std::cos(rr.heading)*d,8,-std::sin(rr.heading)*d},{.55f,16,.65f},{.68f,.41f,.19f},rr.heading,15);
       box(m,p+V3{0,16,0},{19,.55f,.75f},{.68f,.41f,.19f},rr.heading,15);
       box(m,p+V3{std::cos(rr.heading)*8,12,-std::sin(rr.heading)*8},{.08f,8,.08f},{.16f,.19f,.23f},rr.heading,15);
     }
   }
   for(float at:{630.f,1180.f,2070.f,3020.f,3970.f}){
     auto rr=road.at(at);
     for(int side:{-1,1})box(m,road.surface(at,side*6.3f)+V3{0,3.6f,0},{.12f,7.2f,.12f},{.15f,.18f,.24f},rr.heading,15);
     for(int flag=0;flag<17;flag++){
       float d=-6+flag*.75f,h=6.5f+.018f*d*d;
       V3 a=road.surface(at,d)+V3{0,h,0},b=road.surface(at,d+.45f)+V3{0,h,0},c=road.surface(at+.05f,d+.22f)+V3{0,h-.65f,0};
       triangle(m,a,b,c,flag%3?V3{.95f,.68f,.34f}:V3{.23f,.69f,.65f},0);
     }
   }
   for(float site:{660.f,1190.f,2080.f,3010.f})for(int side:{-1,1}){
     auto rr=road.at(site);
     for(float along:{-22.f,22.f}){
       V3 bed=road.surface(site+along,side*16)+V3{0,.39f,0};
       box(m,bed,{4,.48f,7},{.35f,.35f,.32f},rr.heading,2);
       box(m,bed+V3{0,.25f,0},{3.6f,.07f,6.6f},{.15f,.20f,.11f},rr.heading,2);
     }
     for(int post=0;post<12;post++){
       float at=site-40+post*7;V3 p=road.surface(at,side*21.6f);
       box(m,p+V3{0,.73f,0},{.12f,1.1f,.12f},{.15f,.19f,.22f},rr.heading,15);
       if(post<11)box(m,road.surface(at+3.5f,side*21.6f)+V3{0,1.13f,0},{.09f,.09f,7.1f},{.17f,.22f,.25f},rr.heading,15);
     }
   }
 }
}
static void appendBiomeProps(Mesh&m,const Road&road,V3 camera){
 if(biomeInstances.empty()){
   auto add=[&](float at,float d,float h,int kind,bool paved){
     V3 p=road.surface(at,d);
     const auto& contact=biomeContact(road.biome(),kind);float w=h*contact.aspect;
     auto loc=road.locate(p);
     float footprint=road.biome()==2&&kind==0?.25f:w*std::max(contact.u,1-contact.u);
     float clearance=road.biome()==3&&d<0?6.15f:(kind>=18?5.35f:5.8f);
     if(loc.distance<footprint+clearance||!sceneryPlantClear(p,footprint+.1f))return;
     bool finish=loc.s>4000&&loc.distance<24;
     if(finish&&kind<9)return;
     p.y=sceneryHeight(p)-(kind>=18?h*.10f:.015f);
     (void)paved;
     // Street objects hold their orientation relative to their block. They no
     // longer spin individually with the camera, or form mirrored rows.
     biomeInstances.push_back({p,h,kind,road.at(at).heading+(d<0?.20f:-.20f),road.biome()==2&&!finish});
   };
   if(road.biome()==1){
     for(int j=0;j<1440;j++)for(int side:{-1,1}){
       float at=16+j*2.75f,seed=float(j+side*149);
       add(at,side*(7.5f+hash(seed+3)*27),.45f+hash(seed+1)*.8f,j%3==0?1:4,false);
       if(j%3==0)add(at+2,side*(9+hash(seed+5)*17),3.9f+hash(seed+7)*3.4f,j%9==0?3:0,false);
       if(j%2==0)add(at+1,side*(7.3f+hash(seed+6)*4),.85f+hash(seed+13)*.6f,1,false);
       if(j%7==0)add(at+4,side*(10+hash(seed+9)*30),1.2f+hash(seed+11)*1.7f,2,false);
       if(j%29==0)add(at,side*7.8f,2.1f,5,false);
       if(j%3==0)add(at+1.5f,side*(7.5f+hash(seed+41)*6),1.2f+hash(seed+43)*.9f,6,false);
       if(j%11==0)add(at+3,side*(13+hash(seed+45)*7),3.3f+hash(seed+46)*1.4f,7,false);
       if(j%2==0)add(at+3.5f,side*(6.9f+hash(seed+48)*9),.32f+hash(seed+49)*.75f,8,false);
       for(int band=0;band<3;band++){
         float offset=side*(7.2f+band*6.5f+hash(seed+band*21)*3.8f);
         add(at+hash(seed+band)*4,offset,.65f+hash(seed+band+59)*1.05f,12+(j+band)%6,false);
       }
       if(j%2==0)add(at+2,side*(30+hash(seed+32)*27),1.2f+hash(seed+77)*1.1f,12+j%6,false);
     }
     for(float site:{760.f,2020.f,4060.f}){
       add(site-19,-11,2.15f,9,false);add(site+12,-10,3.0f,10,false);
       add(site+35,12,4.3f,11,false);add(site-28,9,2.0f,6,false);
     }
   }else if(road.biome()==2){
     for(int j=0;j<127;j++)for(int side:{-1,1}){
       float at=18+j*32.f+(side<0?0:15);add(at,side*6.3f,3.8f,0,true);
       // Furnish one useful place at a time: curb lamps, shopside planters,
       // then a bus stop OR cafe OR kiosk. Different sides have different uses.
       add(at+12,side*9.2f,2.7f,1,true);
       int scene=(j+(side<0?2:0))%9;
       if(scene==0)add(at+23,side*8.3f,1.6f,7,true);
       if(scene==1)add(at+22,side*8.8f,3.3f,12,true);
       if(scene==2)add(at+22,side*9.0f,2.1f,13,true);
       if(scene==3)add(at+21,side*9.0f,2.7f,14,true);
       if(scene==4)add(at+22,side*8.6f,2.9f,15,true);
       if(scene==5)add(at+21,side*8.8f,2.2f,16,true);
       if(scene==6)add(at+22,side*8.8f,3.0f,17,true);
       if(scene==7)add(at+22,side*8.6f,2.4f,9,true);
       if(scene==8)add(at+21,side*9.0f,2.2f,10,true);
       add(at+5,side*9.3f,1.1f,8,true);
     }
     for(float site:{660.f,1190.f,2080.f,3010.f})for(int side:{-1,1}){
       add(site-21,side*16,2.6f,6,true);add(site-8,side*17,3.0f,10,true);
       add(site+7,side*14,2.6f,11,true);add(site+22,side*17,3.0f,10,true);
       add(site+32,side*13,1.5f,8,true);
       for(float along:{-24.f,-21.f,-18.f,19.f,22.f,25.f}){
         V3 p=road.surface(site+along,side*16)+V3{0,.67f,0};
         biomeInstances.push_back({p,1.55f,8,road.at(site).heading,true});
       }
       add(site,side*19,2.8f,16,true);
       for(float along:{-33.f,-14.f,13.f,34.f})add(site+along,side*19,3.1f,1,true);
     }
     for(int j=0;j<780;j++)for(int side:{-1,1}){
       float at=20+j*5.05f+(side<0?0:2.5f);
       add(at,side*9.5f,.75f+hash(float(j+side))*.4f,8,true);
     }
     // Spectators and service vehicles line the final esplanade.
     for(int j=0;j<18;j++){
       float at=4018+j*7;V3 p=road.surface(at,j%2?10.f:-10.f);p.y=sceneryHeight(p)-.015f;biomeInstances.push_back({p,j%4==0?2.6f:1.8f,j%4==0?10:11});
     }
   }else{
     bool tropical=road.biome()==4;
     // A continuous ankle/knee-high verge, not occasional giant grass tufts.
     // The closest strip stays behind railings and Scottish dry-stone walls.
     for(int j=0;j<4200;j++)for(int side:{-1,1})for(int band=0;band<3;band++){
       float seed=float(j*13+side*271+band*997),at=5+j*.945f+hash(seed)*.7f;
       float edge=road.biome()==3&&side<0?6.85f:6.05f;
       add(at,side*(edge+band*1.25f+hash(seed+3)*.45f),.38f+hash(seed+7)*.28f,18+(j+band*2+(side>0?3:0))%6,false);
     }
     for(int j=0;j<1800;j++)for(int side:{-1,1}){
       float seed=float(j*7+side*193),at=8+j*2.2f+hash(seed)*1.8f;
       for(int band=0;band<4;band++){
         float d=side*(9.5f+band*4.8f+hash(seed+band*17)*3.7f);
         const int jungle[]={1,12,14,1,14,12,4,15},heath[]={1,12,14,4,12,5,14,12};
         int patch=int(hash(seed+band*59)*8)%8;
         int kind=tropical?jungle[patch]:heath[patch];
         // Larger texture clusters build the undergrowth behind the short verge.
         add(at+hash(seed+band+33)*2,d,.85f+hash(seed+band+41)*1.05f,kind,false);
         if(band>0)add(at+1,side*(std::abs(d)+1.6f),.55f+hash(seed+band+55)*.35f,18+(j+band)%6,false);
       }
       if(j%3==0)add(at+2,side*(30+hash(seed+11)*16),tropical?2.3f:2.5f,2,false);
       // Irregular deep stands, with different species and independent sides.
       // Scotland keeps its low open heath; Azura has a genuinely dense canopy.
       if(tropical||j%3==0)for(int band=0;band<(tropical?3:2);band++){
         float d=side*(16+band*15+hash(seed+band*71)*11);
         float landmark=std::fmod(at-105+142,142.f);
         if(band==0&&(landmark<20||landmark>130))continue;
         int kind=tropical?((j+band)%3==0?0:(j+band)%3==1?3:5):((j+band)%3==0?0:(j+band)%3==1?3:16);
         float h=tropical?(kind==0?10.f:kind==3?5.5f:7.f):(kind==0?7.f:4.5f);
         add(at+hash(seed+band+89)*4,d,h*(.8f+hash(seed+band+93)*.5f),kind,false);
       }
       if(j%2==0)for(int band=0;band<3;band++)
         add(at+1,side*(30+band*13+hash(seed+band+106)*8),1.4f+hash(seed+band+109)*1.2f,tropical?((j+band)%3==0?16:(j+band)%3==1?12:14):(j%4?12:14),false);
     }
     for(int j=0;j<28;j++){
       float at=105+j*142.f;int side=j%3==0?1:-1;
       if(tropical){
         int kind=6+j%6;float h=kind==8?5.5f:kind==11?4.2f:2.6f;
         add(at,side*(kind==8?19:14),h,kind,false);
         add(at+18,side*13,2.4f,17,false);
       }else{
         add(at,side*(j%3?19:34),j%3?4.7f:13.f,j%3?6:7,false);
         add(at+22,side*14,j%2?.9f:1.7f,j%2?10:9,false);
         add(at+40,side*10,1.7f,13,false);
         if(j%4==0)add(at+55,-10,2.8f,17,false);
       }
     }
     if(!tropical){add(3280,48,18,8,false);add(3880,35,16,8,false);}
     for(int j=0;j<15;j++){
       float at=4020+j*8;V3 p=road.surface(at,j%2?-11.f:13.f);
       p.y=sceneryHeight(p)-.015f;
       biomeInstances.push_back({p,tropical?(j%2?2.25f:2.05f):1.8f,tropical?(j%2?10:9):(j%2?11:17)});
     }
   }
   // Layered finish gardens: deep silhouettes close the horizon, short shrubs
   // seal their feet. Placement extends beyond at(), which clamps at road end.
   auto finishPlant=[&](V3 p,float h,int kind){
     const auto& contact=biomeContact(road.biome(),kind);float radius=h*contact.aspect*std::max(contact.u,1-contact.u);
     // Keep the entire painted group on the parking apron. Its root alone
     // can be on asphalt while the spectators at either end hang over the
     // retaining edge, several metres above the lower terrain.
     auto end=road.at(road.size());V3 f{std::sin(end.heading),0,std::cos(end.heading)},q{f.z,0,-f.x};
     V3 delta=p-end.p;float along=dot(delta,f),across=dot(delta,q);
     if(along>-142&&along<-4&&std::abs(across)<24)
       p=p+q*(clamp(across,-15+radius+.3f,21-radius-.3f)-across);
     if(road.locate(p).distance<radius+5.8f||!sceneryPlantClear(p,radius+.1f))return;
     p.y=sceneryHeight(p)-.025f;
     biomeInstances.push_back({p,h,kind});
   };
   for(int row=0;row<4;row++)for(int j=0;j<55;j++){
     float seed=float(row*113+j),x=-135+j*5.f+hash(seed)*2,z=9+row*17+hash(seed+13)*7;
     V3 p=woodlandGround(road,z,x);int kind=0;float h=10;
     if(road.biome()==1){kind=j%3?0:3;h=3+hash(seed+4)*3;}
     if(road.biome()==2){kind=1;h=3.5f+hash(seed+4)*2;}
     if(road.biome()==3){kind=j%4==0?3:0;h=11+hash(seed+4)*9;}
     if(road.biome()==4){kind=j%3==0?0:5;h=12+hash(seed+4)*9;}
     finishPlant(p,h,kind);
     if(road.biome()>=3){p=woodlandGround(road,3+row*4.f,x);finishPlant(p,1.4f+hash(seed+20),road.biome()==3?12:14);}
   }
   for(int side:{-1,1})for(int j=0;j<29;j++){
     float at=4018+j*4.7f;V3 p=road.surface(at,side*(25+hash(float(j+side*13))*6));p.y=sceneryHeight(p);
     int kind=road.biome()==1?(j%3?0:3):road.biome()==2?1:road.biome()==3?(j%3?3:0):(j%3?5:0);
     finishPlant(p,road.biome()<=2?3.8f:8+hash(float(j+41))*5,kind);
     if(j%3==0){p=road.surface(at,side*14);finishPlant(p,road.biome()==2?2.5f:road.biome()==3?3.5f:3.f,road.biome()==1?9+(j/3)%3:road.biome()==2?9+(j/3)%3:road.biome()==3?6:6+(j/3)%6);}
   }
 }
 auto endFrame=road.at(road.size());V3 endForward{std::sin(endFrame.heading),0,std::cos(endFrame.heading)},endRight{endForward.z,0,-endForward.x};
 for(const auto& prop:biomeInstances){
   float distance=length(prop.p-camera);if(distance>650)continue;
   auto b=prop.kind>=18?understoryRects[road.biome()-3][prop.kind-18]:biomeProps[road.biome()-1][prop.kind];
   const auto& contact=biomeContact(road.biome(),prop.kind);float h=prop.height,w=h*contact.aspect;
   V3 toward=norm(camera-prop.p),right=prop.fixed?V3{std::cos(prop.heading),0,-std::sin(prop.heading)}:norm(cross({0,1,0},toward));
   V3 endDelta=prop.p-endFrame.p;float along=dot(endDelta,endForward);
   bool arrival=along>-160&&along<0&&std::abs(dot(endDelta,endRight))<24;
   V3 up=arrival?norm(cross(toward,right)):V3{0,1,0};
   float fade=clamp((650-distance)/150,0,1);
   if((road.biome()==2||arrival)&&distance<150&&prop.kind<18){
     float radius=prop.kind==0?.28f:std::min(w*.37f,1.2f);
     sceneryContactShadow(m,prop.p,right,radius,.30f);
   }
   float tint=.93f+.12f*hash(prop.p.x*.73f+prop.p.z*.39f);
   bool flip=hash(prop.p.x+prop.p.z*3)>.5f&&!prop.fixed&&prop.kind>=18;
   groundedArt(m,prop.p,h,b,contact,right,up,(prop.kind>=18?35:prop.kind>=12?33:prop.kind>=6?32:29)+fade*.4f,{tint,tint,tint},flip);
 }
}
