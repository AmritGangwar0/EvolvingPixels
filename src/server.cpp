#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <filesystem>
#include <csignal>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

namespace fs = std::filesystem;
constexpr double MUTATION_RATE=.16, REPRO_HUNGER=90.0;
constexpr int PORT=8091, POP_CAP=1000;
std::mt19937_64 rng(std::random_device{}());
std::mutex worldMutex;
std::atomic<bool> running{true}, paused{false};
struct Entity { int id=0; std::string type; double x=0,y=0,w=36,h=36,speed=70,detection=120,rotation=0,border=4,lifespan=300,age=0,hunger=100; int herbsEaten=0; double born=0; };
std::vector<Entity> animals, herbs;
int nextId=1; double simTime=0; std::string savePath="herbivore_herbs_state.json";
int W=1100,H=700;
double randd(double a,double b){std::uniform_real_distribution<double>d(a,b);return d(rng);} int randi(int a,int b){std::uniform_int_distribution<int>d(a,b);return d(rng);} double clampd(double x,double a,double b){return std::max(a,std::min(b,x));}
bool isAnimal(const Entity&e){return e.type!="herb";}
std::vector<std::string> diet(const std::string&t){if(t=="herbivore")return{"herb"};if(t=="carnivore")return{"herbivore","omnivore"};if(t=="omnivore")return{"herb","herbivore"};if(t=="cannibal")return{"herbivore","carnivore","omnivore","cannibal"};return{};}
bool canEat(const Entity&p,const Entity&q){if(p.id==q.id)return false;if(p.type=="herbivore")return q.type=="herb";if(p.type=="carnivore")return q.type!="carnivore"&&q.type!="cannibal"&&q.type!="herb";if(p.type=="omnivore")return q.type=="herb"||q.type=="herbivore";if(p.type=="cannibal")return q.type!="herb";return false;}
void constrain(Entity&e){e.x=clampd(e.x,e.w/2,std::max(e.w/2,double(W)-e.w/2));e.y=clampd(e.y,e.h/2,std::max(e.h/2,double(H)-e.h/2));}
Entity makeHerb(double x=-1,double y=-1){Entity e;e.id=nextId++;e.type="herb";e.w=e.h=30;e.x=x<0?randd(15,W-15):x;e.y=y<0?randd(15,H-15):y;return e;}
Entity makeAnimal(std::string type,double x=-1,double y=-1,const Entity*parent=nullptr){Entity e;e.id=nextId++;e.type=std::move(type);e.w=parent?clampd(std::round(parent->w*randd(1-MUTATION_RATE,1+MUTATION_RATE)),6,200):36;e.h=parent?clampd(std::round(parent->h*randd(1-MUTATION_RATE,1+MUTATION_RATE)),6,200):36;e.speed=parent?clampd(parent->speed*randd(1-MUTATION_RATE,1+MUTATION_RATE),6,500):randd(40,110);e.detection=parent?clampd(parent->detection*randd(1-MUTATION_RATE,1+MUTATION_RATE),10,600):randd(80,160);e.border=parent?clampd(parent->border*randd(1-MUTATION_RATE,1+MUTATION_RATE),0,80):randd(0,20);e.lifespan=parent?clampd(parent->lifespan*randd(1-MUTATION_RATE,1+MUTATION_RATE),10,5000):randd(180,420);e.x=x<0?randd(e.w/2,W-e.w/2):x;e.y=y<0?randd(e.h/2,H-e.h/2):y;e.rotation=randd(0,6.283185307);e.hunger=parent?80:100;e.born=simTime;return e;}
void fresh(){animals.clear();herbs.clear();nextId=1;simTime=0;for(int i=0;i<20;i++)herbs.push_back(makeHerb());for(int i=0;i<10;i++)animals.push_back(makeAnimal("herbivore"));for(int i=0;i<5;i++)animals.push_back(makeAnimal("carnivore"));for(int i=0;i<5;i++)animals.push_back(makeAnimal("omnivore"));for(int i=0;i<5;i++)animals.push_back(makeAnimal("cannibal"));}
void child(const Entity&p){if(animals.size()>=POP_CAP)return;animals.push_back(makeAnimal(p.type,clampd(p.x+randd(-20,20),p.w/2,W-p.w/2),clampd(p.y+randd(-20,20),p.h/2,H-p.h/2),&p));}
void tick(double dt){std::lock_guard<std::mutex> lock(worldMutex);simTime+=dt;static double herbClock=0;herbClock+=dt;if(herbClock>=.1){herbClock=0;if(herbs.size()<2000)herbs.push_back(makeHerb());}
 for(auto &a:animals){a.age=simTime-a.born;if(a.age>a.lifespan){a.hunger=-1;continue;}Entity* target=nullptr;double best=a.detection;auto d=diet(a.type);for(auto &h:herbs)if(std::find(d.begin(),d.end(),"herb")!=d.end()){double z=std::hypot(h.x-a.x,h.y-a.y);if(z<best){best=z;target=&h;}}for(auto &o:animals){if(o.id==a.id)continue;bool pred=canEat(o,a);double z=std::hypot(o.x-a.x,o.y-a.y);if(pred&&z<a.detection){a.rotation=std::atan2(a.y-o.y,a.x-o.x);target=nullptr;best=-1;break;}if(std::find(d.begin(),d.end(),o.type)!=d.end()&&z<best){best=z;target=&o;}}
 if(best>=0&&target)a.rotation=std::atan2(target->y-a.y,target->x-a.x);else if(!target)a.rotation+=randd(-.025,.025);double vx=std::cos(a.rotation)*a.speed*dt,vy=std::sin(a.rotation)*a.speed*dt;a.x+=vx;a.y+=vy;double ox=a.x,oy=a.y;constrain(a);if(ox!=a.x||oy!=a.y)a.hunger-=.2;a.hunger-=(std::abs(vx)+std::abs(vy))*.0005*((a.w+a.h)/50)*(dt*60);}
 // Eating is resolved from snapshots by ID, avoiding invalid pointers while vectors change.
 for(auto &a:animals){if(a.hunger<=0)continue;for(size_t i=0;i<herbs.size();){double dist=std::hypot(herbs[i].x-a.x,herbs[i].y-a.y);if((a.type=="herbivore"||a.type=="omnivore")&&dist<(a.w+herbs[i].w+a.h+herbs[i].h)/4){herbs.erase(herbs.begin()+i);a.hunger=std::min(100.0,a.hunger+30);if(a.type=="herbivore"){if(++a.herbsEaten>=3){a.herbsEaten=0;child(a);a.hunger=std::max(0.0,a.hunger-40);}}else if(a.hunger>REPRO_HUNGER){child(a);a.hunger-=40;}break;}else ++i;}}
 // Resolve animal collisions, one prey per predator contact.
 std::vector<int> dead;for(size_t i=0;i<animals.size();i++){for(size_t j=i+1;j<animals.size();j++){auto&a=animals[i];auto&b=animals[j];if(a.hunger<=0||b.hunger<=0)continue;if(a.type=="cannibal"&&b.type=="cannibal")continue;if(std::abs(a.x-b.x)<(a.w+b.w)/2&&std::abs(a.y-b.y)<(a.h+b.h)/2){if(canEat(a,b)){b.hunger=-1;a.hunger=std::min(100.0,a.hunger+50);if(a.hunger>REPRO_HUNGER){child(a);a.hunger-=40;}}else if(canEat(b,a)){a.hunger=-1;b.hunger=std::min(100.0,b.hunger+50);if(b.hunger>REPRO_HUNGER){child(b);b.hunger-=40;}}else{double dx=b.x-a.x,dy=b.y-a.y,dist=std::hypot(dx,dy)+.001;double push=2;a.x-=dx/dist*push;a.y-=dy/dist*push;b.x+=dx/dist*push;b.y+=dy/dist*push;constrain(a);constrain(b);}}}}
 animals.erase(std::remove_if(animals.begin(),animals.end(),[](const Entity&e){return e.hunger<=0;}),animals.end());if(animals.size()>=POP_CAP)paused=true;
}
void save(){std::lock_guard<std::mutex> lock(worldMutex);std::ofstream f(savePath);if(!f)return;f<<"{\"simTime\":"<<simTime<<",\"herbs\":[";for(size_t i=0;i<herbs.size();i++){auto&e=herbs[i];if(i)f<<',';f<<"{\"x\":"<<e.x<<",\"y\":"<<e.y<<"}";}f<<"],\"animals\":[";for(size_t i=0;i<animals.size();i++){auto&e=animals[i];if(i)f<<',';f<<"{\"type\":\""<<e.type<<"\",\"x\":"<<e.x<<",\"y\":"<<e.y<<",\"w\":"<<e.w<<",\"h\":"<<e.h<<",\"speed\":"<<e.speed<<",\"detection\":"<<e.detection<<",\"rotation\":"<<e.rotation<<",\"hunger\":"<<e.hunger<<",\"age\":"<<e.age<<",\"lifespan\":"<<e.lifespan<<"}";}f<<"]}";}
std::string stateJson(){std::lock_guard<std::mutex> lock(worldMutex);std::ostringstream s;s<<"{\"paused\":"<<(paused?"true":"false")<<",\"simTime\":"<<simTime<<",\"herbs\":[";for(size_t i=0;i<herbs.size();i++){auto&e=herbs[i];if(i)s<<',';s<<"{\"id\":"<<e.id<<",\"x\":"<<e.x<<",\"y\":"<<e.y<<",\"w\":"<<e.w<<",\"h\":"<<e.h<<"}";}s<<"],\"animals\":[";for(size_t i=0;i<animals.size();i++){auto&e=animals[i];if(i)s<<',';s<<"{\"id\":"<<e.id<<",\"type\":\""<<e.type<<"\",\"x\":"<<e.x<<",\"y\":"<<e.y<<",\"w\":"<<e.w<<",\"h\":"<<e.h<<",\"detection\":"<<e.detection<<",\"rotation\":"<<e.rotation<<",\"hunger\":"<<e.hunger<<",\"age\":"<<e.age<<"}";}s<<"],\"counts\":{\"herbs\":"<<herbs.size();for(auto t:{"herbivore","carnivore","omnivore","cannibal"})s<<",\""<<t<<"\":"<<std::count_if(animals.begin(),animals.end(),[&](auto&e){return e.type==t;});s<<"}}";return s.str();}
std::string readFile(const fs::path&p){std::ifstream f(p,std::ios::binary);if(!f)return{};std::ostringstream s;s<<f.rdbuf();return s.str();}
std::string response(const std::string&body,const std::string&type="application/json; charset=utf-8",int code=200){std::ostringstream s;s<<"HTTP/1.1 "<<code<<(code==200?" OK":" Not Found")<<"\r\nContent-Type: "<<type<<"\r\nContent-Length: "<<body.size()<<"\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET, POST, OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type\r\nConnection: close\r\n\r\n"<<body;return s.str();}
void handleClient(int fd){char buf[8192];int n=recv(fd,buf,sizeof(buf)-1,0);if(n<=0){close(fd);return;}buf[n]='\0';std::istringstream req(buf);std::string method,path,ver;req>>method>>path>>ver;std::string out;if(method=="OPTIONS")out=response("");else if(path=="/api/state")out=response(stateJson());else if(path=="/api/pause"&&method=="POST"){paused=!paused;out=response(std::string("{\"paused\":")+(paused?"true":"false")+"}");}else if(path=="/api/reset"&&method=="POST"){std::lock_guard<std::mutex> lock(worldMutex);fresh();paused=false;out=response("{\"ok\":true}");}else if(path=="/api/save"&&method=="POST"){save();out=response("{\"ok\":true}");}else if(path=="/"||path=="/index.html"){auto b=readFile("public/index.html");out=response(b,"text/html; charset=utf-8");}else if(path=="/style.css"){out=response(readFile("public/style.css"),"text/css; charset=utf-8");}else if(path=="/app.js"){out=response(readFile("public/app.js"),"application/javascript; charset=utf-8");}else out=response("Not found", "text/plain; charset=utf-8",404);send(fd,out.data(),out.size(),0);close(fd);}
int main(){fs::create_directories("public");fresh();if(fs::exists(savePath))std::cerr<<"Existing save found at "<<savePath<<"; starting fresh (load support can be added separately).\n";std::thread sim([]{auto last=std::chrono::steady_clock::now(),saveAt=last;while(running){auto now=std::chrono::steady_clock::now();double dt=std::chrono::duration<double>(now-last).count();last=now;if(!paused)tick(std::min(dt,.06));if(now-saveAt>std::chrono::seconds(5)){saveAt=now;save();}std::this_thread::sleep_for(std::chrono::milliseconds(16));}});int server=socket(AF_INET,SOCK_STREAM,0);if(server<0){perror("socket");running=false;sim.join();return 1;}int opt=1;setsockopt(server,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));sockaddr_in addr{};addr.sin_family=AF_INET;addr.sin_addr.s_addr=INADDR_ANY;addr.sin_port=htons(PORT);if(bind(server,(sockaddr*)&addr,sizeof(addr))<0||listen(server,16)<0){perror("bind/listen");close(server);running=false;sim.join();return 1;}std::cout<<"Herbivore & Herbs server running at http://localhost:"<<PORT<<"\nPress Ctrl+C to stop.\n";while(running){int c=accept(server,nullptr,nullptr);if(c>=0)handleClient(c);}close(server);sim.join();return 0;}
