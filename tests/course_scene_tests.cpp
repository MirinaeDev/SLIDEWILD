#include "world.hpp"
#include "ground_coverage.hpp"
#include "frontend.hpp"
#include <iostream>
#include <stdexcept>
#include <map>
#include <array>
#include "future_clearance.hpp"
#include "arrival_sprite_tests.hpp"
using namespace dh;
int main(int argc,char**argv){try{
 int first=argc>1?std::clamp(std::stoi(argv[1]),1,courseCount-1):1;
 for(int selected=first;selected<courseCount;selected++){
   Frontend menu;menu.page=MenuPage::Globe;menu.country=selected;menu.confirm();for(int k=0;k<36;k++)menu.tick(1.f/60);
   if(menu.page!=MenuPage::Briefing)throw std::runtime_error("New destination remains locked");
   menu.confirm();for(int k=0;k<260;k++)menu.tick(1.f/60);if(!menu.startRequested||menu.country!=selected)throw std::runtime_error("Destination lost at launch");
   Road road(selected);auto world=buildWorld(road);size_t rocks=0,facades=0,ground=0;
   if(selected!=6)checkArrivalSprites(road);
   if(selected==7){
     checkGroundSeams(world,road);
     using Point=std::array<int,3>;using Edge=std::pair<Point,Point>;std::map<Edge,int> edges;
     auto key=[](V3 p){return Point{int(std::round(p.x*10000)),int(std::round(p.y*10000)),int(std::round(p.z*10000))};};
     size_t stone=0,propsCount=0,jets=0;
     for(size_t i=0;i<world.vertices.size();i+=3){
       float mat=world.vertices[i].material;
       if(mat!=50&&mat!=57)continue;
       stone++;
       for(int k=0;k<3;k++){
         auto a=key(world.vertices[i+k].p),b=key(world.vertices[i+(k+1)%3].p);
         if(a==b)throw std::runtime_error("Degenerate basalt mass");
         if(b<a)std::swap(a,b);
         edges[{a,b}]++;
       }
     }
     for(const auto&[edge,count]:edges){(void)edge;if(count!=2)throw std::runtime_error("Open basalt or volcanic gallery");}
     for(float s:{200.f,570.f,1410.f,1550.f,2440.f,3030.f,4050.f,4110.f}){
       Rider r;r.reset(road);r.s=s;r.p=road.surface(s,0);r.runTime=3.3f;
       V3 camera=road.surface(s-4,0)+V3{0,2,0};Mesh dynamic;appendTrees(dynamic,road,r,camera);
       if(dynamic.size()*2+20000>400000)throw std::runtime_error("Volcanic scenery overflows dynamic GPU buffer");
       for(size_t k=0;k<dynamic.size();){
         float mat=dynamic[k].material;
         if(mat>=29&&mat<29.5f){
           V3 foot=(dynamic[k+5].p+dynamic[k+2].p)*.5f;
           if(!sceneryPlantClear(foot,length(dynamic[k+2].p-dynamic[k+5].p)*.5f-.02f))throw std::runtime_error("Volcanic vegetation intersects basalt");
           if(road.locate(foot).distance<5.6f)throw std::runtime_error("Volcanic furniture crosses the road");
           propsCount++;k+=6;
         }else if(mat==54){jets++;k+=6;}else{k+=3;}
       }
     }
     if(stone<12000||propsCount<1000||jets<10)throw std::runtime_error("Sparse volcanic course");
     // Eruption cels must animate without changing their world-space footprint.
     Rider probe;probe.reset(road);probe.s=530;probe.p=road.surface(probe.s,0);
     V3 camera=road.surface(526,0)+V3{0,2,0};Mesh before,after;
     probe.runTime=1.1f;appendTrees(before,road,probe,camera);
     probe.runTime=2.f;appendTrees(after,road,probe,camera);
     std::map<Point,std::array<Vertex,3>> cels;size_t animated=0;
     for(size_t k=0;k<before.size();k+=3)if(before[k].material==54)
       cels[key((before[k].p+before[k+1].p+before[k+2].p)*(1.f/3))]={before[k],before[k+1],before[k+2]};
     for(size_t k=0;k<after.size();k+=3)if(after[k].material==54){
       auto old=cels.find(key((after[k].p+after[k+1].p+after[k+2].p)*(1.f/3)));if(old==cels.end())continue;
       for(int v=0;v<3;v++)if(length(old->second[v].p-after[k+v].p)>.001f)throw std::runtime_error("Eruption cel stretches its billboard");
       if(std::abs(old->second[0].u-after[k].u)+std::abs(old->second[0].v-after[k].v)>.01f)animated++;
     }
     if(animated<2)throw std::runtime_error("Decorative geysers do not animate");
     std::cout<<"PASS Caldeira: "<<stone<<" closed rock/tunnel triangles, "<<propsCount<<" props, "<<jets<<" animated geysers; shoulder coverage, escape lanes and sprite clearance\n";
     continue;
   }
   if(selected==6){
     checkFutureClearance(world,road);
     size_t deck=0,facade=0,lights=0;
     size_t arches=0,archEdges=0,barriers=0,signs=0,panels=0,tiers=0,identifiers=0;
     std::array<int,4> families{};
     for(size_t k=0;k<world.vertices.size();k+=6){
       const auto& a=world.vertices[k];const auto& b=world.vertices[k+1];const auto& foot=world.vertices[k+5];
       if(a.material==64){
         archEdges++;
         if(std::abs(length(a.p-foot.p)-1.36f)>.002f)throw std::runtime_error("Painted gateway has no consistent depth");
       }
       if(a.material==65){
         barriers++;
         if(std::abs((b.u-a.u)-2.f/4.8f)>.0002f)throw std::runtime_error("Barrier texture loses physical scale");
       }
       if(a.material==69){
         signs++;
         if(std::abs(length(b.p-a.p)/length(a.p-foot.p)-6)>.1f)throw std::runtime_error("Direction panel stretches");
       }
       if(a.material==58){
         V3 width=b.p-a.p,height=a.p-foot.p;arches++;
         float sourceRatio=(b.u-a.u)*1659/((foot.v-a.v)*948);
         if(std::abs(dot(norm(width),norm(height)))>.0002f||std::abs(length(width)/length(height)-sourceRatio)>.001f)
           throw std::runtime_error("Painted arch stretches or shears in banked corner");
       }
       if(a.material>=60&&a.material<64&&std::abs(foot.p.y-(road.at(road.size()).p.y-90))>.001f)
         throw std::runtime_error("Distant tower terminates in mid-air");
       if(a.material==30||a.material==59)throw std::runtime_error("Legacy office/residential facade in racing acropolis");
       if(a.material==70){
         panels++;
         float sourceRatio=(b.u-a.u)*1536/((foot.v-a.v)*1024);
         if(std::abs(length(b.p-a.p)/length(a.p-foot.p)-sourceRatio)>.002f)
           throw std::runtime_error("Authored industrial elevation stretches");
         int cell=(a.u>.5f?1:0)+(a.v>.5f?2:0);families[cell]++;
       }
       if(a.material==74)tiers++;
       if(a.material==73)identifiers++;
     }
     if(arches<40||panels<70||tiers<48||identifiers<10||families[0]<20||families[1]<10||families[3]<10)
       throw std::runtime_error("Missing illustrated gates, pit halls, stepped grandstands or service pylons");
     if(archEdges<1000||barriers!=8320||signs<12)throw std::runtime_error("Incomplete gateway depth, rail cladding or corner signage");
     for(size_t k=0;k<world.vertices.size();k+=3){
       const auto& a=world.vertices[k];
       for(int v=0;v<3;v++){auto p=world.vertices[k+v].p;if(!std::isfinite(p.x+p.y+p.z))throw std::runtime_error("Non-finite suspended deck");}
       if(a.material==1)deck++;
       if(a.material==49)lights++;
       if((a.material>=70&&a.material<=74)||(a.material>=60&&a.material<64)){
         facade++;
         V3 center=(a.p+world.vertices[k+1].p+world.vertices[k+2].p)*(1.f/3);
         auto loc=road.locate(center);
         if(loc.distance<5&&center.y>road.surface(loc.s,loc.d).y-.1f)throw std::runtime_error("Futuristic building crosses a track branch");
       }
     }
     if(deck!=4160||facade<2500||lights<100)throw std::runtime_error("Incomplete futuristic world");
     for(float s=3;s<4150;s+=11){
       RoadSurfaceQuery ground(road,s,4);
       for(float d:{-4.3f,0.f,4.3f}){
         V3 p=road.surface(s,d);float y=roadGroundPoint(road,p,s).y;p.y+=30;
         if(!ground.project(p)||std::abs(p.y-y)>.001f)throw std::runtime_error("Hole in banked deck or shadow projection");
       }
     }
     for(float s:{30.f,280.f,1020.f,2050.f,3130.f,4000.f,4110.f}){
       Rider r;r.reset(road);r.s=s;r.p=road.surface(s,0);V3 camera=road.surface(s-4,0)+V3{0,2,0};
       Mesh props;appendTrees(props,road,r,camera);
       if(props.size()<100||props.size()*2+20000>400000)throw std::runtime_error("Future props density/budget");
       for(size_t k=0;k<props.size();k+=6){
         if(props[k].material==7)continue;
         if(props[k].material<29||props[k].material>=29.5f)throw std::runtime_error("Natural biome leaked into future city");
         V3 width=props[k+1].p-props[k].p,height=props[k].p-props[k+5].p;
         float ratio=(props[k+1].u-props[k].u)*1536/((props[k+5].v-props[k].v)*1024);
         if(std::abs(length(width)/length(height)-ratio)>.001f||std::abs(dot(norm(width),norm(height)))>.001f)
           throw std::runtime_error("Future furniture sprite stretches on a bank");
         V3 foot=(props[k+5].p+props[k+2].p)*.5f;auto loc=road.locate(foot);
         if(loc.distance<5.2f)throw std::runtime_error("Future furniture intersects the road");
       }
     }
     std::cout<<"PASS Neon Descent: "<<deck<<" road triangles, "<<facade<<" industrial triangles, "<<panels<<" authored elevations, "<<tiers<<" stepped stand tiers, "<<identifiers<<" pylon identifiers, "<<arches<<" undistorted arch faces; physical panel scale, continuous deck, grounded shadows, furniture clearance, selection\n";
     continue;
   }
   checkGroundSeams(world,road);
   using Point=std::array<int,3>;using Edge=std::pair<Point,Point>;std::map<Edge,std::pair<int,int>> edges;
   auto key=[](V3 p){return Point{int(std::round(p.x*10000)),int(std::round(p.y*10000)),int(std::round(p.z*10000))};};
   for(size_t i=0;i<world.vertices.size();i+=3){
     auto a=world.vertices[i],b=world.vertices[i+1],c=world.vertices[i+2];
     if(a.material!=18&&a.material!=30&&a.material!=31&&a.material!=3)continue;
     V3 center=(a.p+b.p+c.p)*(1.f/3);auto loc=road.locate(center);
     if(loc.distance<4.6f&&center.y>roadGroundPoint(road,center,loc.s).y-.01f)
       throw std::runtime_error("New course scenery crosses a drivable road branch");
     if(a.material==3||a.material==31)ground++;
     if(a.material==30)facades++;
     if(a.material==18){
       rocks++;
       for(int k=0;k<3;k++){
         auto x=key(world.vertices[i+k].p),y=key(world.vertices[i+(k+1)%3].p);
         if(x==y)throw std::runtime_error("Degenerate desert rock");
         int direction=x<y?1:-1;if(y<x)std::swap(x,y);auto&e=edges[{x,y}];e.first++;e.second+=direction;
       }
     }
   }
   for(const auto&[edge,count]:edges){(void)edge;if(count.first!=2||count.second!=0)throw std::runtime_error("Open canyon rock mesh");}
   if(selected==1&&rocks<1000)throw std::runtime_error("Canyon masses missing");
   if(selected==2&&facades<1000)throw std::runtime_error("City facades missing");
   if(selected==5){
     for(float s:{260.f,560.f,1300.f,1550.f,2780.f,4050.f}){
       Rider r;r.reset(road);r.s=s;r.p=road.surface(s,0);auto rr=road.at(s);
       V3 camera=r.p-V3{std::sin(rr.heading)*4,-2,std::cos(rr.heading)*4};Mesh props;appendTrees(props,road,r,camera);
       size_t shadows=0;for(size_t i=0;i<props.size();i+=3){
         float mat=props[i].material;
         bool caster=(mat>=6&&mat<6.5f)||(mat>=13&&mat<13.5f)||(mat>=21&&mat<21.5f);
         if(caster&&length(props[i].p-camera)<160)shadows+=3;
       }
       std::cout<<"Park sample "<<s<<" scenery "<<props.size()<<" shadows "<<shadows<<" reserve "<<props.size()+shadows+20000<<'\n';
       if(props.size()<1000||props.size()+shadows+20000>400000)throw std::runtime_error("Gravity Park scenery density/buffer budget at "+std::to_string(s));
     }
     std::cout<<"PASS Gravity Park: closed rocks, banked road clearance, ground seams, scenery budget\n";continue;
   }
   size_t props=0,lowPlants=0,closeVerge=0,maxDynamic=0,maxFrame=0;
   for(float at:{120.f,500.f,980.f,1850.f,2900.f,3920.f,4050.f,4110.f}){
     Rider r;r.reset(road);r.s=at;r.p=road.at(at).p;auto rr=road.at(at);
     V3 cam=r.p-V3{std::sin(rr.heading)*4,-2,std::cos(rr.heading)*4};Mesh dynamic;appendTrees(dynamic,road,r,cam);
     maxDynamic=std::max(maxDynamic,dynamic.size());
     size_t casterVertices=0;
     for(size_t i=0;i<dynamic.size();i+=3)if(dynamic[i].material!=7&&length(dynamic[i].p-cam)<160)casterVertices+=3;
     // Renderer only duplicates casters inside 160 m, not all 650 m of scenery.
     // Reserve another 20k vertices for four riders, effects and HUD.
     maxFrame=std::max(maxFrame,dynamic.size()+casterVertices+20000);
     for(size_t i=0;i<dynamic.size();i+=6){
       float mat=dynamic[i].material;
       if(mat==7)continue;
       if(!((mat>=29&&mat<29.5f)||(mat>=32&&mat<33.5f)||(mat>=35&&mat<35.5f)))throw std::runtime_error("Alpine vegetation leaked into new biome");
       V3 left=dynamic[i+5].p,right=dynamic[i+2].p,foot=(left+right)*.5f;
       if(!sceneryPlantClear(left,0)||!sceneryPlantClear(right,0)||road.locate(foot).distance<5.2f)
         throw std::runtime_error("New biome sprite overlaps a rock or the road");
       if(mat>=35){
         lowPlants++;
         if(road.locate(foot).distance<9&&length(foot-cam)<80)closeVerge++;
         if(length(dynamic[i].p-left)>.95f)throw std::runtime_error("Roadside cover became tall bushes");
       }
       props++;
     }
   }
   if(props<100)throw std::runtime_error("New biome props missing");
   if(selected>=3&&(lowPlants<4000||closeVerge<500))throw std::runtime_error("Sparse roadside cover in dense biomes");
   if(maxFrame>400000)throw std::runtime_error("Scenery exceeds dynamic buffer including nearby casters and four-rider reserve");
   std::cout<<"DETAIL low plants "<<lowPlants<<", close verge "<<closeVerge<<", max dynamic vertices "<<maxDynamic<<", frame reserve "<<maxFrame<<'\n';
   std::cout<<"PASS course "<<selected<<": "<<ground<<" ground triangles, "<<rocks<<" closed rock triangles, "<<facades<<" facade triangles, "<<props<<" visible props; road clearance and selection\n";
 }
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
