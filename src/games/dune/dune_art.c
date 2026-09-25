/* DUNE EXPRESS - pixel art (palette-letter strings, see gfx.h).
 * One body, facing right, in every pose: h skin, r shirt, y sash, b trousers,
 * e boots. Each outlaw, guard and the Governor recolours it and wears a hat. */
#include "dune.h"

Sprite dx_body[DS_FRAMES];
Sprite dx_obj[DO_COUNT];
Sprite dx_icon[8];

#define TOP \
    "................" \
    "................" \
    "......kkkk......" \
    ".....khhhhk....." \
    "....khhhhhhk...." \
    "....khhhhkhk...." \
    "....khhhhkhk...." \
    "....khhhhhhhk..." \
    ".....khhhhkk...." \
    "......khhk......"
#define TORSO \
    "....kkyyyykk...." \
    "...krryyyyrrk..." \
    "...krrrrrrrrk..." \
    "..krrkrrrrrkrk.." \
    "..krrkrrrrrkrk.." \
    "..krrkrrrrrkrk.." \
    "..khhkrrrrrkhk.." \
    "..khhkrrrrrkhk.." \
    "...kkkyyyyykk..."

static const char STAND[] = TOP TORSO
    "....kbbbbbbk...."
    "....kbbbbbbk...."
    "....kbbkkbbk...."
    "....kbbkkbbk...."
    "....kbbkkbbk...."
    "....kbbkkbbk...."
    "....kbbkkbbk...."
    "....kbbkkbbk...."
    "....keekkeek...."
    "....keeekeeek..."
    "....kkkkkkkkk..."
    "................"
    "................";
static const char WALK1[] = TOP TORSO
    "....kbbbbbbk...."
    "....kbbbbbbk...."
    "...kbbbkkbbbk..."
    "...kbbk..kbbk..."
    "..kbbk....kbbk.."
    "..kbbk....kbbk.."
    ".kbbk......kbbk."
    ".kbbk......kbbk."
    ".keek......keek."
    ".keeek.....keeek"
    ".kkkkk.....kkkkk"
    "................"
    "................";
static const char WALK2[] = TOP TORSO
    "....kbbbbbbk...."
    "....kbbbbbbk...."
    "....kbbbbbk....."
    ".....kbbbbk....."
    ".....kbbbbk....."
    ".....kbkkbk....."
    ".....kbkkbk....."
    ".....kbkkbk....."
    ".....keekek....."
    ".....keeekeek..."
    ".....kkkkkkkk..."
    "................"
    "................";
static const char DUCK[] =
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
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "......kkkk......"
    ".....khhhhk....."
    "....khhhhkhk...."
    "....khhhhkhhk..."
    ".....khhhhkk...."
    "....kkyyyykk...."
    "...krrrrrrrrk..."
    "..krrkrrrrrkrk.."
    "..khhkrrrrrkhk.."
    "..kkkyyyyyykkk.."
    "..kbbbbbbbbbbk.."
    "..kbbbbbbbbbbk.."
    ".keeebbk.keeek.."
    ".kkkkkk..kkkkk.."
    "................"
    "................";
static const char ROLL[] =
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
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    ".....kkkkk......"
    "....krrrrrk....."
    "...krrhhhrrk...."
    "..krrhhhhhrrk..."
    "..krhhhhkhhrk..."
    "..krrhhhhhrrk..."
    "..kybrrrrrrbk..."
    "..kybbrrrrbbk..."
    "...kbbbbbbbk...."
    "....keeeeek....."
    ".....kkkkk......"
    "................"
    "................"
    "................"
    "................";
static const char JUMP[] = TOP
    "..k.kkyyyykk.k.."
    "..khrryyyyrrhk.."
    "..khrrrrrrrrhk.."
    "...krrrrrrrrk..."
    "...krrrrrrrrk..."
    "...krrrrrrrrk..."
    "...krrrrrrrrk..."
    "...krrrrrrrrk..."
    "...kkkyyyyykk..."
    "....kbbbbbbk...."
    "...kbbbbbbbbk..."
    "...kbbbk.kbbk..."
    "..kbbbk..kbbk..."
    "..keek...keeek.."
    "..kkkk...kkkkk.."
    "................"
    "................"
    "................"
    "................"
    "................"
    "................"
    "................";
static const char CLIMB[] =
    "................"
    "..k..........k.."
    ".khk.kkkk...khk."
    ".khkkrrrrk..khk."
    ".khkrrrrrrk.khk."
    "..khrrrrrrrkhk.."
    "..khrrrrrrrrk..."
    "...krrrrrrrrk..."
    "....krrrrrrk...."
    "......krrk......"
    "....kkyyyykk...."
    "...krrrrrrrrk..."
    "...krrrrrrrrk..."
    "...krrrrrrrrk..."
    "...krrrrrrrrk..."
    "...krrrrrrrrk..."
    "...krrrrrrrrk..."
    "...krrrrrrrrk..."
    "...kkkyyyyykk..."
    "....kbbbbbbk...."
    "....kbbbbbbk...."
    "....kbbkkbbk...."
    "....kbbk.kbbk..."
    "....kbbk.kbbk..."
    "...kbbk..kbbk..."
    "...kbbk...kbbk.."
    "...keek...keek.."
    "...kkkk...kkkk.."
    "................"
    "................"
    "................"
    "................";
static const char PUNCH[] = TOP
    "....kkyyyykk...."
    "...krryyyyrrkkkk"
    "...krrrrrrrrrrhk"
    "..krrkrrrrrrrrhk"
    "..krrkrrrrrkkkk."
    "..krrkrrrrrk...."
    "..khhkrrrrrk...."
    "..khhkrrrrrk...."
    "...kkkyyyyyk...."
    "....kbbbbbbk...."
    "....kbbbbbbk...."
    "...kbbbkkbbbk..."
    "...kbbk..kbbk..."
    "..kbbk....kbbk.."
    "..kbbk....kbbk.."
    ".kbbk......kbbk."
    ".kbbk......kbbk."
    ".keek......keek."
    ".keeek.....keeek"
    ".kkkkk.....kkkkk"
    "................"
    "................";
static const char GUN[] = TOP
    "....kkyyyykk...."
    "...krryyyyrrkkkk"
    "...krrrrrrrrhsgk"
    "..krrkrrrrrrkkgk"
    "..krrkrrrrrk..k."
    "..krrkrrrrrk...."
    "..khhkrrrrrk...."
    "..khhkrrrrrk...."
    "...kkkyyyyyk...."
    "....kbbbbbbk...."
    "....kbbbbbbk...."
    "....kbbkkbbk...."
    "....kbbkkbbk...."
    "....kbbkkbbk...."
    "....kbbkkbbk...."
    "....kbbkkbbk...."
    "....kbbkkbbk...."
    "....keekkeek...."
    "....keeekeeek..."
    "....kkkkkkkkk..."
    "................"
    "................";
static const char DOWN[] =
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
    "................"
    "................"
    ".kkkk..........."
    "khhhhkkkkkkk...."
    "khhkhkyrrrrrkkkk"
    "khhhhkyrrrrrbbek"
    ".kkkkkkkkkkkkkkk"
    "................"
    "................"
    "................"
    "................"
    "................";

/* hats and hair (16 x 10), drawn over the head */
static const char HAT_K[] =      /* Khaled: a red bandana, tails flying */
    "................"
    "......rrrr......"
    ".....rrrrrr....."
    "....rrrrrrrr...."
    "..rrmrrrrrr....."
    ".rr.m..........."
    "rr.............."
    "................"
    "................"
    "................";
static const char HAT_V[] =      /* the Veil: an indigo hood and face cloth */
    "......NNNN......"
    ".....NBBBBN....."
    "....NBBBBBBN...."
    "...NBBBBBBBBN..."
    "...NBBB........."
    "...NBBB..NNNN..."
    "...NBBBNBBBBBN.."
    "....NBBBBBBBN..."
    ".....NNBBBBN...."
    "................";
static const char HAT_S[] =      /* Sahar: a magenta headscarf and a braid */
    "......PPPP......"
    ".....PKKKPP....."
    "....PKPPPPPP...."
    "...PPPPPPPPPP..."
    "...PPPP........."
    "..vPPP.........."
    "..vv............"
    "..vk............"
    "...v............"
    "...k............";
static const char HAT_G[] =      /* a rail guard's kepi */
    "................"
    "......NNNN......"
    ".....NNNNNN....."
    ".....NNNNNN....."
    ".....NyyyyNkkk.."
    "................"
    "................"
    ".........kk....."
    "................"
    "................";
static const char HAT_GOV[] =    /* the Governor: a tall kepi, gold braid, moustache */
    "......NNNN......"
    ".....NNNNNN....."
    ".....NyyyyN....."
    ".....NNNNNN....."
    ".....NyyyyNkkk.."
    "................"
    "................"
    ".........kkk...."
    "........kkkkk..."
    "................";

/* things */
static const char BARREL[] =
    "................"
    "...kkkkkkkkk...."
    "..kqqfqqqqfqk..."
    "..kkkkkkkkkkk..."
    "..kqfqqqqfqqk..."
    "..kqfqqqqfqqk..."
    "..kqfqqqqfqqk..."
    "..kqfqqqqfqqk..."
    "..kkkkkkkkkkk..."
    "..kqfqqqqfqqk..."
    "..kqfqqqqfqqk..."
    "..kqfqqqqfqqk..."
    "..kkkkkkkkkkk..."
    "...kkkkkkkkk...."
    "................"
    "................";
static const char CRATE[] =
    "................"
    ".kkkkkkkkkkkkkk."
    ".kheeeeeeeeeehk."
    ".kekeeeeeeeekek."
    ".keekeeeeeekeek."
    ".keeekeeeekeeek."
    ".keeeekeekeeeek."
    ".keeeeekkeeeeek."
    ".keeeeekkeeeeek."
    ".keeeekeekeeeek."
    ".keeekeeeekeeek."
    ".keekeeeeeekeek."
    ".kekeeeeeeeekek."
    ".kheeeeeeeeeehk."
    ".kkkkkkkkkkkkkk."
    "................";
static const char ANVIL[] =
    "................"
    "................"
    "................"
    ".kkkkkkkkkkkkk.."
    "kslllllllllllsk."
    "kssssssssssssskk"
    ".kkksssssssskk.."
    "....kssssssk...."
    "....kssssssk...."
    "...kkssssssskk.."
    "..kssssssssssk.."
    "..kddddddddddk.."
    "..kkkkkkkkkkkk.."
    "................"
    "................"
    "................";
static const char DYNAMITE[] =
    "................"
    "..........y....."
    ".........ya....."
    "........kk......"
    ".......kk......."
    "....kkkkkkk....."
    "...krrrrrrrk...."
    "...krwwrrrrk...."
    "...krrrrrrrk...."
    "...krrrrrrrk...."
    "...krrrrrrrk...."
    "...kyyyyyyyk...."
    "...krrrrrrrk...."
    "....kkkkkkk....."
    "................"
    "................";
static const char GOOSE[] =
    "................"
    "..........kkk..."
    ".........kwwwk.."
    ".........kwkwoo."
    ".........kwwwkoo"
    "..........kwwk.."
    "..........kwwk.."
    "...kkkkkkkwwwk.."
    "..kwwwwwwwwwwk.."
    ".kwwlllwwwwwwk.."
    ".kwlllllwwwwk..."
    "..kwwlllwwwk...."
    "...kkkkkkkk....."
    ".....ko..ko....."
    ".....oo..oo....."
    "................";
static const char GOOSE2[] =
    "................"
    "..........kkk..."
    ".........kwwwk.."
    "..kk.....kwkwoo."
    ".kwwk....kwwwkoo"
    ".kwwwk....kwwk.."
    "..kwwwk...kwwk.."
    "...kwwwkkkwwwk.."
    "...kwwwwwwwwwk.."
    "..kwwwwwwwwwwk.."
    ".kwwwwwwwwwwk..."
    "..kwwwwwwwwk...."
    "...kkkkkkkk....."
    ".....ko..ko....."
    ".....oo..oo....."
    "................";
static const char RAM[] =
    "............................"
    "............................"
    ".kk.................kkkk...."
    "kllkkkkkkkkkkkkkkkkkllllk..."
    "kllllllllllllllllllklkkllk.."
    "kllllllllllllllllllllkggklk."
    ".kllllllllllllllllllllkkggk."
    ".kllllllllllllllllllllllkk.."
    ".klllllllllllllllllllllkk..."
    "..kllllllllllllllllllkkk...."
    "..kllkkkkkkkkkkkkkkllk......"
    "..kgk..kgk......kgk.kgk....."
    "..kgk..kgk......kgk.kgk....."
    "..kkk..kkk......kkk.kkk....."
    "............................"
    "............................";
static const char CRANK[] =
    "................"
    "................"
    "....kkkkk......."
    "...kssssskkkkkkk"
    "...ksllllsssssss"
    "...ksssssskkkkkk"
    "..kkkkkkkkk....."
    "..kekk.kkek....."
    "...kek.kek......"
    "....kekek......."
    ".....kek........"
    "....kekek......."
    "...kek.kek......"
    "..kek...kek....."
    "..kk.....kk....."
    "................";
static const char LEVER[] =
    "................"
    "................"
    "...........kk..."
    "..........krrk.."
    "..........krrk.."
    ".........kssk..."
    "........kssk...."
    ".......kssk....."
    "......kssk......"
    ".....kssk......."
    "...kkkkkkkkkk..."
    "..kggggggggggk.."
    "..kgsssssssggk.."
    "..kggggggggggk.."
    "..kkkkkkkkkkkk.."
    "................";
static const char LOOT[] =
    "................"
    "................"
    "..kkkkkkkkkkkk.."
    ".kttttttttttttk."
    ".ktyyyyyyyyyytk."
    ".kkkkkkkkkkkkkk."
    ".ksssssyyssssk.."
    ".ksssssyysssssk."
    ".ksssskyykssssk."
    ".kssssskkssssskk"
    ".ksssssssssssssk"
    ".kttttttttttttkk"
    ".ktssssssssssstk"
    ".kkkkkkkkkkkkkk."
    "................"
    "................";
static const char COIN[] =
    "..kkkk.."
    ".kyyyyk."
    "kyycyyak"
    "kycyyyak"
    "kyyyyyak"
    "kyyyyaak"
    ".kaaaak."
    "..kkkk..";
static const char AMMO[] =
    "..k..k.."
    ".kyk.kyk"
    ".kyk.kyk"
    ".kak.kak"
    "kkkkkkkk"
    "kttttttk"
    "kttttttk"
    "kkkkkkkk";
static const char POWER[] =
    "kkkkkkkk"
    "kyyyyyyk"
    "kyCCCCyk"
    "kyCIICyk"
    "kyCIICyk"
    "kyCCCCyk"
    "kyyyyyyk"
    "kkkkkkkk";
static const char CAMEL[] =
    "................................"
    "........................kkk....."
    ".......................khhhk...."
    "......................khhhhhkk.."
    "......................khhkhhhhk."
    "......................khhhhhhhhk"
    "......................khhhhkkkk."
    ".......kkkk..........khhhhk....."
    "......khhhhk.........khhhk......"
    ".....khhhhhhkkk......khhhk......"
    "....khhhhhhhhhhk....khhhhk......"
    "...khhhhhhhhhhhhk..khhhhk......."
    "..kkhhhhhhhhhhhhhkkhhhhk........"
    ".kPkhhhhhhhhhhhhhhhhhhk........."
    "kPkPhhhhhhyyyyyyhhhhhk.........."
    "kPkkhhhhhhyryryyhhhhhk.........."
    ".k.khhhhhhyyyyyyhhhhhk.........."
    "...khhhhhhhhhhhhhhhhhk.........."
    "....khhhhhhhhhhhhhhhk..........."
    ".....khhhkkkkkkkkhhhk..........."
    "......khk........khk............"
    "......khk........khk............"
    "......khk.......khk............."
    "......khk.......khk............."
    ".....khk.........khk............"
    ".....khk.........khk............"
    ".....khk..........khk..........."
    ".....khk..........khk..........."
    "....kkkk.........kkkk..........."
    "....kkkkk........kkkkk.........."
    "................................"
    "................................";
static const char CAMEL2[] =
    "................................"
    "........................kkk....."
    ".......................khhhk...."
    "......................khhhhhkk.."
    "......................khhkhhhhk."
    "......................khhhhhhhhk"
    "......................khhhhkkkk."
    ".......kkkk..........khhhhk....."
    "......khhhhk.........khhhk......"
    ".....khhhhhhkkk......khhhk......"
    "....khhhhhhhhhhk....khhhhk......"
    "...khhhhhhhhhhhhk..khhhhk......."
    "..kkhhhhhhhhhhhhhkkhhhhk........"
    ".kPkhhhhhhhhhhhhhhhhhhk........."
    "kPkPhhhhhhyyyyyyhhhhhk.........."
    "kPkkhhhhhhyryryyhhhhhk.........."
    ".k.khhhhhhyyyyyyhhhhhk.........."
    "...khhhhhhhhhhhhhhhhhk.........."
    "....khhhhhhhhhhhhhhhk..........."
    ".....khhhkkkkkkkkhhhk..........."
    ".....khk..........khk..........."
    "....khk............khk.........."
    "....khk............khk.........."
    "...khk..............khk........."
    "...khk..............khk........."
    "..khk................khk........"
    "..khk................khk........"
    "..khk................khk........"
    ".kkkk................kkkk......."
    ".kkkkk...............kkkkk......"
    "................................"
    "................................";

/* the power-up icons (8 x 8): spare rounds, blast rounds, spring boots,
 * the hamsa, the iron fist, the quick holster */
static const char IC_ROUNDS[] =
    "..k..k.."
    ".kyk.kyk"
    ".kyk.kyk"
    ".kak.kak"
    "kkkkkkkk"
    "kttttttk"
    "kttttttk"
    "kkkkkkkk";
static const char IC_BLAST[] =
    "..kkkk.."
    ".koyyok."
    "koyrryok"
    "kyrwwryk"
    "kyrwwryk"
    "koyrryok"
    ".koyyok."
    "..kkkk..";
static const char IC_BOOTS[] =
    ".kkk...."
    ".kbbk..."
    ".kbbk..."
    ".kbbk..."
    ".kbbkkk."
    "kbbbbbbk"
    "kyykyyyk"
    ".kk.kkk.";
static const char IC_CHARM[] =
    ".k.k.k.."
    "kukukuk."
    "kuuuuuk."
    "kuuwuuk."
    "kuwkwuk."
    ".kuuuk.."
    "..kuk..."
    "...k....";
static const char IC_FIST[] =
    ".kkkkk.."
    "khkhkhk."
    "khhhhhkk"
    "khhhhhhk"
    "khhhhhhk"
    ".khhhhk."
    ".kssssk."
    ".kkkkkk.";
static const char IC_QUICK[] =
    "......kk"
    ".....kyk"
    "kkkkkyk."
    "kssskk.."
    ".kgk...."
    ".kgk...."
    "..k....."
    "........";

static void remap_none(Sprite *s, const char *src, int w, int h) { spr_make(s, w, h, src); }

void dx_art_load(void) {
    if (dx_body[DS_STAND].px) return;
    remap_none(&dx_body[DS_STAND], STAND, 16, 32);
    remap_none(&dx_body[DS_WALK1], WALK1, 16, 32);
    remap_none(&dx_body[DS_WALK2], WALK2, 16, 32);
    remap_none(&dx_body[DS_DUCK], DUCK, 16, 32);
    remap_none(&dx_body[DS_ROLL], ROLL, 16, 32);
    remap_none(&dx_body[DS_JUMP], JUMP, 16, 32);
    remap_none(&dx_body[DS_CLIMB], CLIMB, 16, 32);
    remap_none(&dx_body[DS_PUNCH], PUNCH, 16, 32);
    remap_none(&dx_body[DS_GUN], GUN, 16, 32);
    remap_none(&dx_body[DS_DOWN], DOWN, 16, 32);
    spr_make(&dx_obj[DO_BARREL], 16, 16, BARREL);
    spr_make(&dx_obj[DO_CRATE], 16, 16, CRATE);
    spr_make(&dx_obj[DO_ANVIL], 16, 16, ANVIL);
    spr_make(&dx_obj[DO_DYNAMITE], 16, 16, DYNAMITE);
    spr_make(&dx_obj[DO_GOOSE], 16, 16, GOOSE);
    spr_make(&dx_obj[DO_GOOSE2], 16, 16, GOOSE2);
    spr_make(&dx_obj[DO_RAM], 28, 16, RAM);
    spr_make(&dx_obj[DO_GUN], 16, 16, CRANK);
    spr_make(&dx_obj[DO_LEVER], 16, 16, LEVER);
    spr_make(&dx_obj[DO_LOOT], 16, 16, LOOT);
    spr_make(&dx_obj[DO_COIN], 8, 8, COIN);
    spr_make(&dx_obj[DO_AMMO], 8, 8, AMMO);
    spr_make(&dx_obj[DO_POWER], 8, 8, POWER);
    spr_make(&dx_obj[DO_CAMEL], 32, 32, CAMEL);
    spr_make(&dx_obj[DO_CAMEL2], 32, 32, CAMEL2);
    spr_make(&dx_obj[DO_HAT_K], 16, 10, HAT_K);
    spr_make(&dx_obj[DO_HAT_V], 16, 10, HAT_V);
    spr_make(&dx_obj[DO_HAT_S], 16, 10, HAT_S);
    spr_make(&dx_obj[DO_HAT_G], 16, 10, HAT_G);
    spr_make(&dx_obj[DO_HAT_GOV], 16, 10, HAT_GOV);
    static const char *const IC[6] = {IC_ROUNDS, IC_BLAST, IC_BOOTS, IC_CHARM, IC_FIST, IC_QUICK};
    for (int i = 0; i < 8; i++) spr_make(&dx_icon[i], 8, 8, IC[i % 6]);
}
