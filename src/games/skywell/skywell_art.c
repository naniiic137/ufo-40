/* SKYWELL - pixel art (palette-letter strings, see gfx.h). All drawn for UFO 40. */
#include "skywell.h"

Sprite sw_spr[K_SPRITE_COUNT];

static const char STAND[] =
    "......kkkk......"
    ".....kbbbbk....."
    "....kbbbbbbk...."
    "....kbhhhhbk...."
    "....kkIIkIIk...."
    "....khhhhhhk...."
    ".....khhhhk....."
    "....koooooook..."
    "...kBBoBBBBBk..."
    "...khBBBBBBhk..."
    "....kBBBBBBk...."
    "....kBByyBBk...."
    "....kNNkkNNk...."
    "....kNNk.kNk...."
    "....kkk...kk...."
    "................";
static const char RUN1[] =
    "......kkkk......"
    ".....kbbbbk....."
    "....kbbbbbbk...."
    "....kbhhhhbk...."
    "....kkIIkIIk...."
    "....khhhhhhk...."
    ".....khhhhk....."
    "...ooooooook...."
    "..ookBBBBBBk...."
    "....khBBBBBhk..."
    "....kBBBBBBk...."
    "....kBByyBBk...."
    "...kNNk.kNNk...."
    "..kNNk...kNk...."
    "..kkk....kk....."
    "................";
static const char RUN2[] =
    "......kkkk......"
    ".....kbbbbk....."
    "....kbbbbbbk...."
    "....kbhhhhbk...."
    "....kkIIkIIk...."
    "....khhhhhhk...."
    ".....khhhhk....."
    "..oooooooook...."
    "....kBBBBBBk...."
    "...khBBBBBBhk..."
    "....kBBBBBBk...."
    "....kBByyBBk...."
    ".....kNNNNk....."
    ".....kNkkNk....."
    ".....kk..kk....."
    "................";
static const char JUMP[] =
    "......kkkk......"
    ".....kbbbbk....."
    "....kbbbbbbk...."
    "....kbhhhhbk...."
    "..h.kkIIkIIk.h.."
    "..k.khhhhhhk.k.."
    "..kk.khhhhk.kk.."
    "...kkoooooookk.."
    "....kBBBBBBok..."
    "....kBBBBBBk.o.."
    "....kBBBBBBk...."
    "....kBByyBBk...."
    "...kNNk..kNNk..."
    "...kNk....kNk..."
    "...kk......kk..."
    "................";
static const char FALL[] =
    "......kkkk......"
    ".....kbbbbk....."
    "....kbbbbbbk...."
    "....kbhhhhbk...."
    "....kkIIkIIk...."
    ".h..khhhhhhk..h."
    ".kk..khhhhk..kk."
    "..kkooooooookk.."
    "....kBBBBBBk.oo."
    "....kBBBBBBk...."
    "....kBBBBBBk...."
    "....kBByyBBk...."
    "....kNNkkNNk...."
    "...kNNk..kNNk..."
    "...kk......kk..."
    "................";
static const char SHOOT_UP[] =
    ".......kk......."
    "......kggk......"
    "......kbbkk....."
    "....kbbbbbbk...."
    "....kbhhhhbk...."
    "....kkIIkIIk...."
    "....khhhhhhk...."
    ".....khhhhkh...."
    "....kooooookh..."
    "...kBBoBBBBBk..."
    "....kBBBBBBk...."
    "....kBByyBBk...."
    "....kNNkkNNk...."
    "....kNNk.kNk...."
    "....kkk...kk...."
    "................";
static const char STUN[] =
    "................"
    "......kkkk......"
    ".....kbbbbk....."
    "....kbbbbbbk...."
    "....kbhhhhbk...."
    "....kkykkkyk...."
    "....khhhhhhk...."
    ".....khkkhk....."
    "..h.koooooooh..."
    "..kkBBBBBBBkk..."
    "....kBBBBBBk...."
    "....kBByyBBk...."
    "...kNNk..kNNk..."
    "..kNNk....kNNk.."
    "..kk........kk.."
    "................";
static const char STAR[] =
    ".....y.kkkk.y..."
    ".....kbbbbk....."
    "....kbbbbbbk...."
    "..y.kbhhhhbk.y.."
    "....kkIIkIIk...."
    "..h.khhhhhhk.h.."
    "..kk.khhhhk.kk.."
    "...kkoooooookk.."
    "....kBBBBBBk...."
    "....kBBBBBBk...."
    "....kBBBBBBk...."
    "....kBByyBBk...."
    ".....kNNNNk....."
    ".....kNkkNk....."
    "......k..k......"
    ".....y....y.....";
static const char DEAD[] =
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "..kkkk.........."
    ".kbbbbkkkkkkk..."
    "kbhhhhkBBBBBBkk."
    "kkIIkIkooBBNNNk."
    ".khhhhkkkkkkkkk."
    "..kkkk..........";
static const char CLOUD[] =
    "...kkkk....kkk.."
    "..kwwwwk..kwwwk."
    ".kwwwwwwkkwwwwwk"
    "kwwlwwwwwwwwwwlk"
    "kwlllwwwwwlwwllk"
    "kllllllllllllllk"
    ".kgllggllllggkk."
    "..kkkkkkkkkkkk.."
    "................"
    "................";
static const char CRATE[] =
    "kkkkkkkkkkkkkkkk"
    "keettteettteettk"
    "kettbbtetbbttetk"
    "kttbtttbttbbttek"
    "ketbbtetttbbtetk"
    "keettteettteettk"
    "kbbbbbbbbbbbbbbk"
    "kkkkkkkkkkkkkkkk";
static const char ROCK[] =
    ".kkkkkkkkkkkkkk."
    "kggggsgggggsgggk"
    "kgsgggggsgggggsk"
    "ksggssgggsggsggk"
    "kggsggsggggsggsk"
    "ksgggggssgggggsk"
    "kssssssssssssssk"
    ".kkkkkkkkkkkkkk.";
static const char STARBLOCK[] =
    ".......kk......."
    "......kyyk......"
    "......kyyk......"
    ".....kyyyyk....."
    "kkkkkkyyyykkkkkk"
    "kyyyyyyyyyyyyyyk"
    ".kyyyyyccyyyyyk."
    "..kyyyyccyyyyk.."
    "...kyyyyyyyyk..."
    "...kyyyyyyyyk..."
    "..kyyyykkyyyyk.."
    "..kyyyk..kyyyk.."
    ".kyyyk....kyyyk."
    ".kyyk......kyyk."
    ".kkk........kkk."
    "................";
static const char COINBLOCK[] =
    "kkkkkkkkkkkkkkkk"
    "kyyyyyyyyyyyyyak"
    "kyakkkkkkkkkkyak"
    "kyakyyyykkaakyak"
    "kyakyakkakaakyak"
    "kyakkkkkkkkkkyak"
    "kaaaaaaaaaaaaaak"
    "kkkkkkkkkkkkkkkk";
static const char MINE[] =
    "..k..kkkk..k...."
    "...kkssssk.k...."
    "..ksssdsssk..k.."
    ".ksssdddsssk...."
    "kksssyssssssk..."
    ".kssyyysskssk..."
    "..kssyssssssk.k."
    ".k.kkssssssk...."
    "....k.kkkkk.k..."
    "....r..y..r....."
    "................"
    "................";
static const char BOMB[] =
    "kkkkkkkkkkkkkkkk"
    "krrrrrrrrrrrrrrk"
    "krkkkrrrrrrkkkrk"
    "krkykrrrrrrkykrk"
    "krkkkrrrrrrkkkrk"
    "krrrrrrrrrrrrrrk"
    "kmmmmmmmmmmmmmmk"
    "kkkkkkkkkkkkkkkk";
static const char ZAPBLOCK[] =
    "kkkkkkkkkkkkkkkk"
    "klllllllllllllgk"
    "klkkkkkkkkkkkkgk"
    "klkCCCCyyCCCCkgk"
    "klkkkkkkkkkkkkgk"
    "kggggggggggggggk"
    "kssssssssssssssk"
    "kkkkkkkkkkkkkkkk";
static const char BUBBLE[] =
    ".....kkkkkk....."
    "...kkIIIIIIkk..."
    "..kIIwwIIIIIIk.."
    ".kIIwIIIIIIIIIk."
    "kIIIIIIIIIIIIIIk"
    "kCIIIIIIIIIIIICk"
    ".kCCIIIIIIIICCk."
    "..kkCCCCCCCCkk.."
    "....kkkkkkkk...."
    "................";
static const char METAL[] =
    "kkkkkkkkkkkkkkkk"
    "kllllllllllllllk"
    "klggggggggggggsk"
    "klgkgggggggkgsk."
    "klggggggggggggsk"
    "kssssssssssssssk"
    "kIkIkIkIkIkIkIkk"
    "kkkkkkkkkkkkkkkk";
static const char COIN1[] =
    ".kkkk."
    "kyaayk"
    "kakkak"
    "kakkak"
    "kyaayk"
    ".kkkk.";
static const char COIN2[] =
    "..kk.."
    ".kyak."
    ".kaak."
    ".kaak."
    ".kyak."
    "..kk..";
static const char KEY[] =
    "..kkk..."
    ".kyyyk.."
    "kyk.kyk."
    "kyk.kyk."
    ".kyyyk.."
    "..kyk..."
    "..kyk..."
    "..kyyk.."
    "..kyk..."
    "..kyyk..";
static const char BAT_SLEEP[] =
    "....kkkk...."
    "...kVVVVk..."
    "..kVVVVVVk.."
    "..kVkVVkVk.."
    "..kVVVVVVk.."
    "...kVVVVk..."
    "...kpkkpk..."
    "....k..k...."
    "............"
    "............";
static const char BAT1[] =
    "k..........k"
    "kk..kkkk..kk"
    "kVk.kVVk.kVk"
    "kVVkVyyVkVVk"
    "kVVVVVVVVVVk"
    ".kVVVkkVVVk."
    "..kk.kk.kk.."
    ".....kk....."
    "............"
    "............";
static const char BAT2[] =
    "............"
    "....kkkk...."
    "....kVVk...."
    "..kkVyyVkk.."
    ".kVVVVVVVVk."
    "kVVVVkkVVVVk"
    "kVVkkkkkkVVk"
    "kkk..kk..kkk"
    "............"
    "............";
static const char MINNOW1[] =
    "........kkkkkkk........."
    ".....kkkIIIIIIIkkk....kk"
    "...kkIIIIIIIIIIIIIkk.kIk"
    "..kIIkIIIIIIIIIIIIIIkIIk"
    ".kIIkkIIIIIIIIIIIIIIIIIk"
    "..kCCCCCCCCCCCCCCCCkkCCk"
    "...kkCCCCCCCCCCCCkk..kCk"
    ".....kkkkkkkkkkkk.....kk";
static const char MINNOW2[] =
    "........kkkkkkk........."
    ".....kkkIIIIIIIkkk......"
    "...kkIIIIIIIIIIIIIkk..kk"
    "..kIIkIIIIIIIIIIIIIIkkIk"
    ".kIIkkIIIIIIIIIIIIIIIIIk"
    "..kCCCCCCCCCCCCCCCCkkCCk"
    "...kkCCCCCCCCCCCCkk.kCCk"
    ".....kkkkkkkkkkkk....kkk";
static const char JELLY[] =
    "...kkkkkk..."
    "..kKKKKKKk.."
    ".kKwwKKKKKk."
    ".kKwKKKKKKk."
    "kKKKKKKKKKKk"
    "kPPPPPPPPPPk"
    ".kkPkPPkPkk."
    "..kPk.kPk..."
    "..kP..kP.k.."
    "..k...k..P.."
    ".k...k...k.."
    "............";
static const char JELLY_SMALL[] =
    "..kkkk.."
    ".kKwKKk."
    "kKKKKKKk"
    "kPPPPPPk"
    ".kPkkPk."
    ".kP..kP."
    "..k...k."
    "........";
static const char SQUID1[] =
    "....kkkk...."
    "...kpppppk.."
    "..kpVVVVpk.."
    "..kpVkVkpk.."
    "..kpVVVVpk.."
    "..kppppppk.."
    ".kpkpkpkpk.."
    ".kp.kp.kp..."
    ".k..k..k...."
    "............"
    "............"
    "............";
static const char SQUID2[] =
    "....kkkk...."
    "...kpppppk.."
    "..kpVVVVpk.."
    "..kpVkVkpk.."
    "..kpVVVVpk.."
    "..kppppppk.."
    "..kpkpkpkpk."
    "...kp.kp.kp."
    "....k..k..k."
    "............"
    "............"
    "............";
static const char OWL1[] =
    ".kk........kk..."
    ".kek......kek..."
    "..kekkkkkkek...."
    "..keeeeeeeek...."
    ".keyykeekyyek..."
    ".keykkeekkyek..."
    ".keeeeaaeeeek..."
    "kkteeeeeeeetkk.."
    "ktttteeeettttk.."
    ".kttteeeetttk..."
    "..kkkkaakkkk...."
    ".....k..k.......";
static const char OWL2[] =
    ".kk........kk..."
    ".kek......kek..."
    "..kekkkkkkek...."
    "..keeeeeeeek...."
    ".keyykeekyyek..."
    ".keykkeekkyek..."
    ".keeeeaaeeeek..."
    "..keeeeeeeek...."
    ".kteeeeeeeetk..."
    "kttttkaakttttk.."
    "kkkk.k..k.kkkk.."
    "................";
static const char EYE[] =
    ".........kkkkkkkkkk........."
    "......kkkPPPPPPPPPPkkk......"
    "....kkPPPwwwwwwwwwwPPPkk...."
    "...kPPwwwwwwwwwwwwwwwwPPk..."
    "..kPwwwwwwwwwwwwwwwwwwwwPk.."
    ".kPwwwwwwwwwkkkkwwwwwwwwwPk."
    ".kPwwwwwwwkkrrrrkkwwwwwwwPk."
    "kPwwwwwwwkrrrrrrrrkwwwwwwwPk"
    "kPwwwwwwkrrrrrrrrrrkwwwwwwPk"
    "kPwwwwwwkrrrrrrrrrrkwwwwwwPk"
    "kPwwwwwwkrrrrrrrrrrkwwwwwwPk"
    "kPwwwwwwkrrrrrrrrrrkwwwwwwPk"
    "kPwwwwwwkrrrrrrrrrrkwwwwwwPk"
    "kPwwwwwwwkrrrrrrrrkwwwwwwwPk"
    ".kPwwwwwwwkkrrrrkkwwwwwwwPk."
    ".kPwwwwwwwwwkkkkwwwwwwwwwPk."
    "..kPwwwwwwwwwwwwwwwwwwwwPk.."
    "...kPPwwwwwwwwwwwwwwwwPPk..."
    "....kkPPPwwwwwwwwwwPPPkk...."
    "......kkkPPPPPPPPPPkkk......"
    ".........kkkkkkkkkk........."
    "............................"
    "............................"
    "............................";
static const char EYE_SHUT[] =
    "............................"
    "............................"
    "............................"
    "............................"
    "............................"
    ".........kkkkkkkkkk........."
    "......kkkPPPPPPPPPPkkk......"
    "....kkPPPPPPPPPPPPPPPPkk...."
    "..kkPPPPPPPPPPPPPPPPPPPPkk.."
    "kkPPPPPPPPPPPPPPPPPPPPPPPPkk"
    "kPPkkkkkkkkkkkkkkkkkkkkkkPPk"
    "kPkPkPkPkPkPkPkPkPkPkPkPkkPk"
    ".kPPPPPPPPPPPPPPPPPPPPPPPPk."
    "..kkPPPPPPPPPPPPPPPPPPPPkk.."
    "....kkPPPPPPPPPPPPPPPPkk...."
    "......kkkPPPPPPPPPPkkk......"
    ".........kkkkkkkkkk........."
    "............................"
    "............................"
    "............................"
    "............................"
    "............................"
    "............................"
    "............................";
static const char HAND[] =
    "..kk.kk.kk......"
    ".kPPkPPkPPk....."
    ".kPPkPPkPPkkk..."
    ".kPPkPPkPPkPPk.."
    ".kPPPPPPPPkPPk.."
    ".kPPPPPPPPPPk..."
    "..kPPPPPPPPk...."
    "..kPPPPPPPk....."
    "...kPPPPPk......"
    "...kPPPPPk......"
    "...kVVVVVk......"
    "....kVVVk......."
    "....kkkkk......."
    "................";
static const char HAND_OPEN[] =
    ".k..k..k..k....."
    "kPkkPkkPkkPk...."
    "kPkkPkkPkkPk...."
    "kPPkPPkPPkPPk..."
    ".kPPPPPPPPPPk..."
    ".kPPPPPPPPPPk..."
    "..kPPPPPPPPk...."
    "..kPPPPPPPPk...."
    "...kPPPPPPk....."
    "...kPPPPPk......"
    "...kVVVVVk......"
    "....kVVVk......."
    "....kyyyk......."
    ".....kyk........";
static const char TICKMINE[] =
    "..kkkk.."
    ".krrrrk."
    "krrkkrrk"
    "krkyykrk"
    "krkyykrk"
    "krrkkrrk"
    ".krrrrk."
    "..kkkk..";
static const char SHOT[] =
    ".yy."
    "ywwy"
    "ywwy"
    ".yy.";
static const char ORB[] =
    "..kk.."
    ".kPPk."
    "kPwKPk"
    "kPKPPk"
    ".kPPk."
    "..kk..";
static const char TINKER[] =
    "........kkkkkk.........."
    ".......kbbbbbbk........."
    "......kbbbbbbbbk........"
    ".....kkkkkkkkkkkk......."
    "......kttttttttk........"
    "......ktkkttkktk........"
    "......kttttttttk........"
    "......kttteettk........."
    ".......kttttttk........."
    ".....kkggggggggkk......."
    "....kggglllllgggggk....."
    "...kgglyylllyylllggk...."
    "...kgglyylllyyllllgk...."
    "...kggllllllllllllgk...."
    "....kggggggggggggggk...."
    "....kbbbbbbbbbbbbbbk...."
    "...kbkkbbbbbbbbkkbbk...."
    "...kk..kkkkkkkk..kkk...."
    "..kbbk.........kbbk....."
    "...kk...........kk......";
static const char KIP_BIG[] =
    ".........kkkkkk........."
    ".......kkbbbbbbkk......."
    "......kbbbbbbbbbbk......"
    ".....kbbbbbbbbbbbbk....."
    ".....kbbhhhhhhhhbbk....."
    ".....kkkIIIkkIIIkkk....."
    ".....kkIIwIkkIIwIkk....."
    ".....kkkIIIkkIIIkkk....."
    ".....khhhhhhhhhhhhk....."
    "......khhhhkkhhhhk......"
    ".......khhhhhhhhk......."
    ".....kooooooooooooook..."
    "....kooooooooooooooook.."
    "...kBBBBooBBBBBBBBkook.."
    "...kBBBBBBBBBBBBBBBk.o.."
    "..khBBBBBBBBBBBBBBBhk..."
    "..khkBBBBBBBBBBBBBkhk..."
    "...kkBBBBBByyBBBBBkk...."
    ".....kBBBByyyyBBBBk....."
    ".....kBBBBBBBBBBBBk....."
    ".....kNNNNNkkNNNNNk....."
    ".....kNNNNNkkNNNNNk....."
    ".....kNNNNk..kNNNNk....."
    ".....kNNNNk..kNNNNk....."
    "....kkkkkkk..kkkkkkk...."
    "....kbbbbbk..kbbbbbk...."
    "....kkkkkkk..kkkkkkk...."
    "........................";
static const char GRINDER[] =
    "....kk......kk.."
    "...klk.....klk.."
    "..klllk...klllk."
    ".kllllk..kllllk."
    "kggggggggggggggk"
    "krrrrrrrrrrrrrrk"
    "kmmmmmmmmmmmmmmk"
    "kkkkkkkkkkkkkkkk";

typedef struct { int id, w, h; const char *px; } Art;
static const Art ART[] = {
    {K_STAND, 16, 16, STAND},
    {K_RUN1, 16, 16, RUN1},
    {K_RUN2, 16, 16, RUN2},
    {K_JUMP, 16, 16, JUMP},
    {K_FALL, 16, 16, FALL},
    {K_SHOOT_UP, 16, 16, SHOOT_UP},
    {K_STUN, 16, 16, STUN},
    {K_STAR, 16, 16, STAR},
    {K_DEAD, 16, 16, DEAD},
    {K_CLOUD, 16, 10, CLOUD},
    {K_CRATE, 16, 8, CRATE},
    {K_ROCK, 16, 8, ROCK},
    {K_STARBLOCK, 16, 16, STARBLOCK},
    {K_COINBLOCK, 16, 8, COINBLOCK},
    {K_MINE, 16, 12, MINE},
    {K_BOMB, 16, 8, BOMB},
    {K_ZAPBLOCK, 16, 8, ZAPBLOCK},
    {K_BUBBLE, 16, 10, BUBBLE},
    {K_METAL, 16, 8, METAL},
    {K_COIN1, 6, 6, COIN1},
    {K_COIN2, 6, 6, COIN2},
    {K_KEY, 8, 10, KEY},
    {K_BAT_SLEEP, 12, 10, BAT_SLEEP},
    {K_BAT1, 12, 10, BAT1},
    {K_BAT2, 12, 10, BAT2},
    {K_MINNOW1, 24, 8, MINNOW1},
    {K_MINNOW2, 24, 8, MINNOW2},
    {K_JELLY, 12, 12, JELLY},
    {K_JELLY_SMALL, 8, 8, JELLY_SMALL},
    {K_SQUID1, 12, 12, SQUID1},
    {K_SQUID2, 12, 12, SQUID2},
    {K_OWL1, 16, 12, OWL1},
    {K_OWL2, 16, 12, OWL2},
    {K_EYE, 28, 24, EYE},
    {K_EYE_SHUT, 28, 24, EYE_SHUT},
    {K_HAND, 16, 14, HAND},
    {K_HAND_OPEN, 16, 14, HAND_OPEN},
    {K_TICKMINE, 8, 8, TICKMINE},
    {K_SHOT, 4, 4, SHOT},
    {K_ORB, 6, 6, ORB},
    {K_TINKER, 24, 20, TINKER},
    {K_KIP_BIG, 24, 28, KIP_BIG},
    {K_GRINDER, 16, 8, GRINDER},
};

void sw_art_load(void) {
    static int loaded;
    if (loaded) return;
    for (int i = 0; i < ARRAY_LEN(ART); i++) spr_make(&sw_spr[ART[i].id], ART[i].w, ART[i].h, ART[i].px);
    loaded = 1;
}
