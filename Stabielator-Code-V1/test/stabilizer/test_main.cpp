/**
 * @file test_main.cpp
 * @brief Controleert houdingskalibratie, hysterese, instellingen en veiligheidslimieten.
 * @date 2026-11-08
 * @author ProtoIoT Wietse Houwers
 * @note Prototype software for evaluation use only.
 */
#include "../../src/stabilizer_logic.h"
#include <cassert>
#include <limits>

Quaternion multiply(const Quaternion& a, const Quaternion& b) {
  return {a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z,
          a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
          a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
          a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w};
}
Quaternion rotation(float roll, float pitch=0, float yaw=0) {
  const float k=0.00872664626f;
  Quaternion x{std::cos(roll*k),std::sin(roll*k),0,0};
  Quaternion y{std::cos(pitch*k),0,std::sin(pitch*k),0};
  Quaternion z{std::cos(yaw*k),0,0,std::sin(yaw*k)};
  return multiply(z,multiply(y,x));
}
void off(const StabilizerLogic& logic) { for(auto level:logic.outputs)assert(level==0); }
void calibrate(StabilizerLogic& logic, uint32_t start=0, Quaternion reference={}) {
  logic.settings.onDegrees=10; logic.settings.offDegrees=7; logic.settings.strength=127;
  logic.settings.pattern=0; logic.settings.maxFeedbackMs=0; logic.settings.bootDelayMs=0; logic.settings.calibrationMs=500; logic.begin(start);
  for(unsigned n=0;n<=25;++n) {
    auto q=reference;
    if(n%2){q.w=-q.w;q.x=-q.x;q.y=-q.y;q.z=-q.z;}
    logic.update(start+n*20,q,true,true,n);off(logic);
  }
  assert(logic.referenceValid);assert(logic.state==StabilizerState::Active);
}
int main() {
  constexpr unsigned PLUS_X=3, MINUS_Y=0;
  StabilizerLogic s;
  assert(validSettings(s.settings));
  assert(s.settings.motorMap[0]==3&&s.settings.motorMap[1]==1&&s.settings.motorMap[2]==2&&s.settings.motorMap[3]==0);
  assert(s.settings.onDegrees==1.5f && s.settings.offDegrees==1.0f);
  assert(s.settings.strength==64 && s.settings.pattern==1 && s.settings.diagonal==1);
  assert(s.settings.bootDelayMs==3000 && s.settings.maxFeedbackMs==3000);
  assert(s.settings.pulseOnMs==200 && s.settings.pulseOffMs==200);
  assert(s.settings.inactivityMinutes==60 && s.settings.activityDegrees==1);
  // Oud record heeft andere grootte; lees eerst de versie en reset bewust.
  unsigned char legacy[64]={}; uint32_t version=2; std::memcpy(legacy,&version,4);
  StabilizerSettings saved; saved.onDegrees=15; bool reset=false;
  assert(decodeSettingsRecord(legacy,sizeof(legacy),saved,reset) && reset);
  assert(saved.version==5 && saved.onDegrees==1.5f && saved.strength==64);
  version=1;std::memcpy(legacy,&version,4);
  assert(decodeSettingsRecord(legacy,sizeof(legacy),saved,reset) && reset);
  saved.onDegrees=4;saved.offDegrees=2;
  StabilizerSettings decoded;
  assert(decodeSettingsRecord(&saved,sizeof(saved),decoded,reset) && !reset && decoded.onDegrees==4);
  assert(!decodeSettingsRecord(&saved,sizeof(saved)-1,decoded,reset));
  // Versie 3: behoud alle waarden; de oude paddingbyte is geen helderheid.
  saved.version=3;saved.ledBrightness=255;saved.strength=93;saved.ledEnabled=0;
  assert(decodeSettingsRecord(&saved,sizeof(saved),decoded,reset) && reset);
  assert(decoded.version==5 && decoded.ledBrightness==100 && decoded.onDegrees==4 && decoded.offDegrees==2);
  assert(decoded.strength==93 && decoded.ledEnabled==0 && decoded.motorMap[0]==saved.motorMap[0]);
  decoded.ledBrightness=25;
  assert(decodeSettingsRecord(&decoded,sizeof(decoded),saved,reset) && !reset && saved.ledBrightness==25);
  saved.version=4;saved.debugEnabled=1;saved.testMode=255;
  assert(decodeSettingsRecord(&saved,sizeof(saved),decoded,reset) && reset);
  assert(decoded.version==5 && decoded.debugEnabled==1 && decoded.testMode==1);
  saved=decoded;saved.version=3;saved.testMode=255;
  assert(decodeSettingsRecord(&saved,sizeof(saved),decoded,reset) && reset);
  assert(decoded.debugEnabled==1 && decoded.testMode==0);
  saved.version=999;assert(!decodeSettingsRecord(&saved,sizeof(saved),decoded,reset));
  auto invalid=s.settings;invalid.offDegrees=invalid.onDegrees;assert(!validSettings(invalid));
  invalid=s.settings;invalid.ledBrightness=101;assert(!validSettings(invalid));
  invalid=s.settings;invalid.testMode=2;assert(!validSettings(invalid));
  invalid=s.settings;invalid.motorMap[1]=0;assert(!validSettings(invalid));
  invalid=s.settings;invalid.onDegrees=NAN;assert(!validSettings(invalid));
  invalid=s.settings;invalid.batteryCriticalVolts=3.8f;assert(!validSettings(invalid));
  // De opstartvertraging wordt daadwerkelijk gerespecteerd.
  s.begin(0);s.update(2999,{},true,true,1);assert(s.state==StabilizerState::Waiting);off(s);
  s.update(3000,{},true,true,2);assert(s.state==StabilizerState::Calibrating);off(s);
  // Een herhaald sample mag het minimumaantal metingen niet halen.
  for(unsigned n=0;n<100;++n)s.update(3000+n*20,{},true,true,2);
  assert(!s.referenceValid);assert(s.calibrationSamples==1);
  // Beweging reset het hele meetvenster.
  s.update(7100,rotation(8),true,true,3);assert(s.calibrationSamples==1);
  // Doorlopend heen-en-weer bewegen mag geen referentie opleveren.
  StabilizerLogic rocking;rocking.settings.bootDelayMs=0;rocking.begin(0);
  for(unsigned n=0;n<=100;++n) {
    rocking.update(n*20,rotation(n==0?0:n%2?2:-2),true,true,n);
    assert(!rocking.referenceValid);off(rocking);
  }
  // Na daadwerkelijk stilhouden kan hetzelfde apparaat wel kalibreren.
  for(unsigned n=101;n<=155;++n)rocking.update(n*20,rotation(2),true,true,n);
  assert(rocking.referenceValid);
  StabilizerLogic lowThreshold;lowThreshold.settings.bootDelayMs=0;
  lowThreshold.settings.onDegrees=.1f;lowThreshold.settings.offDegrees=.05f;lowThreshold.begin(0);
  for(unsigned n=0;n<=100;++n) {
    lowThreshold.update(n*20,rotation(n%2?.06f:0),true,true,n);
    assert(!lowThreshold.referenceValid);
  }
  calibrate(s);
  s.update(600,rotation(11),true,true,30);assert(s.outputs[PLUS_X]==127);
  s.update(620,rotation(8),true,true,31);assert(s.outputs[PLUS_X]==127); // hysterese
  s.update(640,rotation(6),true,true,32);off(s);
  s.update(660,rotation(-11),true,true,33);assert(s.outputs[1]==127);assert(!s.outputs[PLUS_X]);
  s.update(680,rotation(0,12),true,true,34);assert(s.outputs[2]==127);assert(!s.outputs[1]);
  s.update(700,rotation(0,-12),true,true,35);assert(s.outputs[MINUS_Y]==127);
  s.update(720,rotation(15,15),true,true,36);assert(s.outputs[PLUS_X]==127&&s.outputs[2]==127);
  s.settings.diagonal=0;s.clearDirections();
  s.update(740,rotation(20,12),true,true,37);assert(s.outputs[PLUS_X]==127&&!s.outputs[2]);
  // Pure yaw geeft geen feedback.
  s.update(760,rotation(0,0,90),true,true,38);off(s);
  s.update(780,rotation(20),false,true,39);off(s);assert(s.state==StabilizerState::SensorFault);
  s.update(800,{NAN,0,0,0},true,true,40);off(s);
  s.update(820,rotation(20),true,false,41);off(s);assert(s.state==StabilizerState::HardwareFault);
  s.pause(true,840);s.update(860,rotation(20),true,true,42);off(s);assert(s.state==StabilizerState::Paused);
  s.pause(false,880);s.update(900,rotation(20),true,true,43);assert(s.outputs[PLUS_X]==127);
  // Pulspatroon zonder sleeps; limiet vergrendelt tot terugkeer onder off-grens.
  calibrate(s);s.settings.pattern=1;s.settings.pulseOnMs=100;s.settings.pulseOffMs=200;
  s.update(600,rotation(20),true,true,30);assert(s.outputs[PLUS_X]==127);
  s.update(700,rotation(20),true,true,31);off(s);
  s.update(900,rotation(20),true,true,32);assert(s.outputs[PLUS_X]==127);
  s.settings.maxFeedbackMs=400;
  s.update(1000,rotation(20),true,true,33);off(s);
  s.update(1600,rotation(20),true,true,34);off(s);
  s.update(1620,rotation(0),true,true,35);off(s);
  s.update(1640,rotation(20),true,true,36);assert(s.outputs[PLUS_X]==127);
  // Willekeurige vaste montagepositie wordt de referentie.
  StabilizerLogic mounted;const auto reference=rotation(35,-25,70);calibrate(mounted,0,reference);
  const auto relative=mounted.relativeOrientation(multiply(reference,rotation(15)));
  assert(std::fabs(quaternionDot(relative,rotation(15)))>.99999f);
  mounted.update(600,multiply(reference,rotation(15)),true,true,30);
  assert(std::fabs(mounted.roll-15)<.05f);assert(mounted.outputs[PLUS_X]==127);
  mounted.update(620,multiply(rotation(0,0,90),reference),true,true,31);off(mounted);
  assert(std::fabs(mounted.tiltRoll)<.05f&&std::fabs(mounted.tiltPitch)<.05f);
  // Instelbare motorkoppeling en assenrotatie.
  mounted.settings.motorMap[0]=0;mounted.settings.motorMap[3]=3;mounted.clearDirections();
  mounted.update(640,multiply(reference,rotation(15)),true,true,32);assert(mounted.outputs[MINUS_Y]==127);
  StabilizerLogic rotated;calibrate(rotated);rotated.settings.axisRotationDegrees=90;
  rotated.update(600,rotation(15),true,true,30);assert(rotated.outputs[2]==127&&!rotated.outputs[PLUS_X]);
  for(int r=-90;r<=90;r+=30)for(int p=-90;p<=90;p+=30) {
    StabilizerLogic arbitrary;const auto ref=rotation(float(r),float(p),45);
    calibrate(arbitrary,0,ref);
    arbitrary.update(600,multiply(rotation(0,0,80),ref),true,true,30);
    off(arbitrary);assert(std::isfinite(arbitrary.tiltRoll)&&std::isfinite(arbitrary.tiltPitch));
  }
  // Alleen doorlopend ontvangen, unieke geldige samples tellen als stilstand.
  StabilizerLogic timed;calibrate(timed);timed.settings.inactivityMinutes=1;
  for(uint32_t t=600;t<=60000;t+=200)timed.update(t,{},true,true,t);
  off(timed);assert(timed.state==StabilizerState::Inactive);
  StabilizerLogic moving;calibrate(moving);moving.settings.inactivityMinutes=1;
  for(uint32_t t=600;t<=120000;t+=200) {
    moving.update(t,rotation(0,0,float((t/10000)%2)*2),true,true,t);
    assert(moving.state!=StabilizerState::Inactive);
  }
  StabilizerLogic missing;calibrate(missing);missing.settings.inactivityMinutes=1;
  for(uint32_t t=600;t<59900;t+=100)missing.update(t,{},true,true,t);
  missing.update(60000,{},false,true,60000);assert(missing.state==StabilizerState::SensorFault);
  missing.update(60100,{},true,true,60100);assert(missing.state!=StabilizerState::Inactive);
  for(uint32_t t=60200;t<120100;t+=100)missing.update(t,{},true,true,t);
  assert(missing.state!=StabilizerState::Inactive);
  missing.update(120100,{},true,true,120100);assert(missing.state==StabilizerState::Inactive);
  StabilizerLogic repeated;calibrate(repeated);repeated.settings.inactivityMinutes=1;
  for(uint32_t t=600;t<=120000;t+=100)repeated.update(t,{},true,true,30);
  assert(repeated.state!=StabilizerState::Inactive);
  StabilizerLogic disabled;calibrate(disabled);disabled.settings.inactivityMinutes=0;
  for(uint32_t t=600;t<=3700000;t+=200)disabled.update(t,{},true,true,t);
  assert(disabled.state==StabilizerState::Active);
  // Max-limiet overleeft meetverlies, motorfout en het opslaan van instellingen.
  StabilizerLogic limited;calibrate(limited);limited.settings.maxFeedbackMs=100;
  limited.update(600,rotation(20),true,true,30);assert(limited.outputs[PLUS_X]==127);
  limited.update(700,rotation(20),true,true,31);off(limited);
  limited.update(720,rotation(20),false,true,32);off(limited);
  limited.update(740,rotation(20),true,true,33);off(limited);
  auto changed=limited.settings;changed.ledEnabled=0;
  assert(limited.applySettings(changed));
  limited.update(760,rotation(20),true,true,34);off(limited);
  limited.update(780,rotation(20),true,false,35);off(limited);
  limited.update(800,rotation(20),true,true,36);off(limited);
  limited.pause(true,810);limited.pause(false,820);
  limited.update(840,rotation(20),true,true,37);off(limited);
  limited.update(860,rotation(0),true,true,38);off(limited);
  limited.update(880,rotation(20),true,true,39);assert(limited.outputs[PLUS_X]==127);
  // Ook voor verstrijken van de limiet mogen meetgaten/opslaan de begintijd niet verschuiven.
  StabilizerLogic interrupted;calibrate(interrupted);interrupted.settings.maxFeedbackMs=100;
  interrupted.update(600,rotation(20),true,true,30);
  interrupted.update(620,rotation(20),false,true,31);off(interrupted);
  changed=interrupted.settings;changed.debugEnabled=1;assert(interrupted.applySettings(changed));
  interrupted.update(700,rotation(20),true,true,32);off(interrupted);
  // Het afgesproken standaardprofiel: lage hoeken, 50%, 200/200, 3 s envelop.
  StabilizerLogic defaults;calibrate(defaults);defaults.settings=StabilizerSettings{};
  defaults.update(600,rotation(1.6f,1.6f),true,true,30);
  assert(defaults.outputs[3]==64 && defaults.outputs[2]==64);
  defaults.update(800,rotation(1.6f,1.6f),true,true,31);off(defaults);
  defaults.update(1000,rotation(1.6f,1.6f),true,true,32);assert(defaults.outputs[3]==64);
  defaults.update(3600,rotation(1.6f,1.6f),true,true,33);off(defaults);
  defaults.update(3800,rotation(1.6f,1.6f),true,true,34);off(defaults);
  defaults.update(4000,rotation(.5f,.5f),true,true,35);off(defaults);
  defaults.update(4200,rotation(1.6f,1.6f),true,true,36);assert(defaults.outputs[3]==64);
  StabilizerLogic hysteresis;calibrate(hysteresis);hysteresis.settings=StabilizerSettings{};
  hysteresis.update(600,rotation(1.6f),true,true,30);assert(hysteresis.outputs[3]==64);
  hysteresis.update(620,rotation(1.2f),true,true,31);assert(hysteresis.outputs[3]==64);
  hysteresis.update(640,rotation(.9f),true,true,32);off(hysteresis);
  hysteresis.pause(true,660);
  hysteresis.update(680,{},false,true,33);off(hysteresis);
  assert(hysteresis.state==StabilizerState::SensorFault); // Ook in ontwikkeltest/pauze een echte fout.
  // Pauze/hervatten bewijst geen beweging en verlengt de stilstandstijd niet.
  StabilizerLogic paused;calibrate(paused);paused.settings.inactivityMinutes=1;
  for(uint32_t t=600;t<30000;t+=200)paused.update(t,{},true,true,t);
  paused.pause(true,30000);
  for(uint32_t t=30000;t<50000;t+=200)paused.update(t,{},true,true,t);
  paused.pause(false,50000);
  for(uint32_t t=50000;t<=60000;t+=200)paused.update(t,{},true,true,t);
  assert(paused.state==StabilizerState::Inactive);
  // unsigned tijdsverschillen blijven correct over millis-overloop.
  StabilizerLogic wrap;calibrate(wrap,0xFFFFFE00U);
  wrap.update(200,rotation(20),true,true,30);assert(wrap.outputs[PLUS_X]==127);
}
