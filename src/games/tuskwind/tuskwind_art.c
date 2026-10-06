/* TUSKWIND - pixel art (palette-letter strings, see gfx.h). All drawn for
 * UFO 40. The islets, lighthouses, stalls, signs, the door, the sea and
 * the sky are drawn in tuskwind_draw.c. */
#include "tuskwind.h"

Sprite tkw_spr[SP_COUNT];

/* Burl, 20 x 14, facing right: a brown walrus in a blue nightcap */
static const char BURL[] =
    "...........kkkk....."
    "..........kBBBBk...."
    ".........kBBBBBBkw.."
    "......kkkkkkkkkkkww."
    "....kkteeeeeeeeek..."
    "...ketteeeeeeeekeek."
    "..kettteeeeeeeeeehhk"
    ".kettttteeeeeeehhhhk"
    ".ketttttteeeeehkhhkk"
    "kettttttttteeekckck."
    "kettttttttttteekckk."
    "ketttbbttttttbbkck.."
    ".kkkbbbkkkkkkbbbkk.."
    "...kkkk.....kkkk....";

static const char BURL_WALK[] =
    "...........kkkk....."
    "..........kBBBBk...."
    ".........kBBBBBBkw.."
    "......kkkkkkkkkkkww."
    "....kkteeeeeeeeek..."
    "...ketteeeeeeeekeek."
    "..kettteeeeeeeeeehhk"
    ".kettttteeeeeeehhhhk"
    ".ketttttteeeeehkhhkk"
    "kettttttttteeekckck."
    "kettttttttttteekckk."
    "kettbbttttttbbtkck.."
    ".kkbbbkkkkkkbbbkkk.."
    "..kkkk.....kkkk.....";

/* squashed down, gathering for the jump */
static const char BURL_CROUCH[] =
    "...................."
    "...........kkkk....."
    "..........kBBBBk...."
    ".........kBBBBBBkw.."
    ".....kkkkkkkkkkkkww."
    "...kkteeeeeeeeeek..."
    "..ketteeeeeeeeekeek."
    ".kettteeeeeeeeeeehhk"
    "kettttteeeeeeeehhhhk"
    "kettttttteeeeehkhhkk"
    "ketttttttttteeekckck"
    "kettbbbttttttbbbkck."
    "kkkbbbbkkkkkkbbbbk.."
    "..kkkkk....kkkkk....";

/* flippers out, flapping */
static const char BURL_FLAP1[] =
    "...........kkkk....."
    "..........kBBBBk...."
    ".........kBBBBBBkw.."
    "......kkkkkkkkkkkww."
    "....kkteeeeeeeeek..."
    "...ketteeeeeeeekeek."
    "..kettteeeeeeeeeehhk"
    ".kettttteeeeeeehhhhk"
    "kkettttteeeeeehkhhkk"
    "bkttttttttteeekckck."
    "bbktttttttttteekckk."
    ".bbktttttttttekck..."
    "..bbkkkkkkkkkkkk...."
    "...bb...........bb..";

static const char BURL_FLAP2[] =
    "...........kkkk....."
    "..........kBBBBk...."
    "bb.......kBBBBBBkw.."
    ".bb...kkkkkkkkkkkww."
    "..bbkkteeeeeeeeek..."
    "...bkteeeeeeeeekeek."
    "..kbttteeeeeeeeeehhk"
    ".kettttteeeeeeehhhhk"
    ".ketttttteeeeehkhhkk"
    "kettttttttteeekckck."
    "kettttttttttteekckk."
    "ketttttttttttttkck.."
    ".kkkkkkkkkkkkkkkk..."
    "....................";

/* fast asleep (the title and the endings) */
static const char BURL_SLEEP[] =
    "...................."
    "...........kkkk....."
    "..........kBBBBk...."
    ".........kBBBBBBkw.."
    ".....kkkkkkkkkkkkww."
    "...kkteeeeeeeeeek..."
    "..ketteeeeeeeekkek.."
    ".kettteeeeeeeeeeehhk"
    "kettttteeeeeeeehhhhk"
    "kettttttteeeeehkhhkk"
    "ketttttttttteeekckck"
    "kettbbbttttttbbbkck."
    "kkkbbbbkkkkkkbbbbk.."
    "..kkkkk....kkkkk....";

/* tumbling */
static const char BURL_FALL[] =
    "bb.........kkkk...bb"
    ".bb.......kBBBBk.bb."
    "..b......kBBBBBBkw.."
    "......kkkkkkkkkkkww."
    "....kkteeeeeeeeek..."
    "...ketteeeeeeeewkeek"
    "..kettteeeeeeeeeehhk"
    ".kettttteeeeeeehhhhk"
    ".ketttttteeeeehkkkkk"
    "kettttttttteeekchch."
    "kettttttttttteekckk."
    "ketttttttttttttkk..."
    ".kkkkkkkkkkkkkkk...."
    "....................";

static const char COCKLE[] =
    "..kkk.."
    ".kKcKk."
    "kKcKcKk"
    "kcKcKck"
    ".kKcKk."
    "..kkk..";

static const char WHELK[] =
    "....kk..."
    "...kak..."
    "..kaaak.."
    ".kaoyaak."
    ".kayoyak."
    "kaayyoaak"
    "kaoaaaook"
    ".kkooookk"
    "...kkkk..";

static const char KEY[] =
    ".kkk....."
    "kyayk...."
    "ky.ykkkkk"
    "kyayyyyyk"
    ".kkk.kk.k";

static const char SPRAT[] =
    "..kkkkk.kk"
    ".kICCCCkCk"
    "kIkCCCBBkk"
    ".kCBBBBkCk"
    "..kkkkk.kk";

static const char SPIRAL[] =
    "..kkk.."
    ".kPPPk."
    "kPKkKPk"
    "kPkKkPk"
    "kPKKPPk"
    ".kPPPk."
    "..kkk..";

static const char CHEST[] =
    ".kkkkkkkkkkkk."
    "kttttttttttttk"
    "ktbbbbbbbbbbtk"
    "kkkkkkkkkkkkkk"
    "ktttttyyyttttk"
    "ktttttykyttttk"
    "ktbbbbbbbbbbtk"
    "kttttttttttttk"
    "kkkkkkkkkkkkkk"
    ".k..........k.";

static const char CHEST_OPEN[] =
    ".kkkkkkkkkkkk."
    "kbbbbbbbbbbbbk"
    "kttttttttttttk"
    ".kkkkkkkkkkkk."
    "kyyaKyyaKyyyak"
    "ktttttykyttttk"
    "ktbbbbbbbbbbtk"
    "ktttttttttttk."
    "kkkkkkkkkkkkkk"
    ".k..........k.";

static const char LIGHTHOUSE[] = /* only the lamp room; the tower is drawn */
    "...kkkkkk..."
    "..kvvvvvvk.."
    ".kkkkkkkkkk."
    ".kyyyyyyyyk."
    ".kyywwwwyyk."
    ".kyyyyyyyyk."
    "kkkkkkkkkkkk";

static const char TERN1[] =
    "kk.....kk"
    "kwk...kwk"
    ".kwkkkwk."
    "..kwwwkrk"
    "...kwwkk."
    "....kk...";

static const char TERN2[] =
    "....kk..."
    "...kwwkrk"
    "..kwwwkk."
    ".kwkkkwk."
    "kwk...kwk"
    "kk.....kk";

/* the Duchess Auk: a tall black-and-white seabird with a little crown */
static const char DUCHESS[] =
    "....y.y.y..."
    "....yyyyy..."
    "...kkkkkkk.."
    "..kkkkkkwkk."
    "..kkwkkkkkyk"
    "..kkkkkkkkyy"
    "..kkkkkkkk.."
    ".kkwwkkkkk.."
    ".kwwwwkkkk.."
    "kkwwwwwkkkk."
    "kkwwwwwkkkk."
    "kkwwwwwkkk.."
    ".kwwwwwkk..."
    "..kwwwkk...."
    "...kkkk....."
    "...a..a.....";

static const char BELL[] =
    "...kkk..."
    "..kaaak.."
    ".kayyyak."
    ".kayyyak."
    ".kayyyak."
    "kaayyyaak"
    "kkkkkkkkk"
    "....k....";

static const char BELL_DONE[] =
    "...kkk..."
    "..ksssk.."
    ".ksgggsk."
    ".ksgggsk."
    ".ksgggsk."
    "kssgggssk"
    "kkkkkkkkk"
    "....k....";

/* a ram, 18 x 12, facing right */
static const char RAM[] =
    "..........kkkk...."
    "...kkkkkkklllkkk.."
    "..kllllllllllkahk."
    ".kllllllllllkahhak"
    ".kllllllllllkhkhhk"
    "kllllllllllllkhhhk"
    "kllllllllllllkkkk."
    "kllllllllllllk...."
    ".klllllllllllk...."
    "..kkhkkkkkhkk....."
    "..khk....khk......"
    "..kk.....kk.......";

static const char RAM_RUN[] =
    "..........kkkk...."
    "...kkkkkkklllkkk.."
    "..kllllllllllkahk."
    ".kllllllllllkahhak"
    ".kllllllllllkhkrhk"
    "kllllllllllllkhhhk"
    "kllllllllllllkkkk."
    "kllllllllllllk...."
    ".klllllllllllk...."
    ".kkhkkkkkkkhkk...."
    ".khk......khk....."
    "kk.........kk.....";

/* a manta, 24 x 8, two wingbeats */
static const char MANTA1[] =
    "kk........kkkk........kk"
    "kVk.....kkVVVVkk.....kVk"
    ".kVVkkkkVVVVVVVVkkkkVVk."
    "..kVVVVVVVwkkwVVVVVVVk.."
    "...kVVVVVVVVVVVVVVVVk..."
    "....kkkkkVVVVVVkkkkk...."
    ".........kkpVkk........."
    "...........kpk..........";

static const char MANTA2[] =
    "..........kkkk.........."
    "........kkVVVVkk........"
    "...kkkkkVVVVVVVVkkkkk..."
    ".kkVVVVVVVwkkwVVVVVVVkk."
    "kVVVVVVVVVVVVVVVVVVVVVVk"
    "kVkkkkkkkVVVVVVkkkkkkkVk"
    "kk.......kkpVkk.......kk"
    "...........kpk..........";

/* Skerry, the old one: a great pale walrus, 28 x 20, facing left */
static const char ELDER[] =
    "............................"
    ".........kkkkkkk............"
    ".......kkIIIIIIIkk.........."
    "......kIIIIIIIIIIIkk........"
    ".....kIIkIIIIIIIIIIIkk......"
    "....kllIIIIIIIIIIIIIIIk....."
    "...kllllIIIIIIIIIIIIIIIk...."
    "..klkllllIIIIIIIIIIIIIIIk..."
    "..kllkllkIIIIIIIIIIIIIIIIk.."
    "..kckkckIIIIIIIIIIIIIIIIIk.."
    "..kck.ckIIIIIIIIIIIIIIIIIIk."
    "..kck.ckIIIIIIIIIIIIIIIIIIk."
    "..kck.ckCIIIIIIIIIIIIIIIIIk."
    "...k..kCCCIIIIIIIIIIIIIIICk."
    "......kCCCCCIIIIIIIIIIICCCk."
    ".....kCCCCCCCCCCCCCCCCCCCCk."
    "....kuuCCkkkkkkkkkkkkCCCuuk."
    "....kuuuk...........kuuuk..."
    ".....kkk.............kkk...."
    "............................";

/* item icons, 9 x 9 */
static const char I_BOBBER[] =
    "...kkk..."
    ".kkrrwkk."
    ".krrrwwk."
    "kwwrrwwwk"
    "kwwwrrrwk"
    "kwwwrrrrk"
    ".krrwwrk."
    ".kkrwwkk."
    "...kkk...";
static const char I_SPYGLASS[] =
    "........."
    ".......kk"
    "......kak"
    "....kkak."
    "...kayk.."
    "..kaak..."
    ".kttk...."
    "kttk....."
    "kkk......";
static const char I_GRAPNEL[] =
    "....k...."
    "...klk..."
    "....k...."
    "....k...."
    "k...k...k"
    "kl..k..lk"
    ".kl.k.lk."
    "..kllk..."
    "...kk....";
static const char I_KITE[] =
    "....k...."
    "...kPk..."
    "..kPPKk.."
    ".kPPPKKk."
    "kkkkkkkkk"
    ".kKKKPPk."
    "..kKKPk.."
    "...kPk..."
    "....k.r.r";
static const char I_SPINNER[] =
    "kkkk.kkkk"
    "kyyykyyyk"
    ".kkkakkk."
    "....k...."
    "..kkBkk.."
    ".kBBBBBk."
    "kBBuuBBBk"
    "kBuuBBBBk"
    ".kkkkkkk.";
static const char I_TIN[] =
    "..kkkkkk."
    ".kgllllgk"
    "kllgggllk"
    "kCCCCCCCk"
    "kBuCCuCBk"
    "kCCCCCCCk"
    "kllgggllk"
    ".kgllllgk"
    "..kkkkkk.";
static const char I_CROSS[] =
    "kk.....kk"
    "krk...krk"
    ".krk.krk."
    "..krkrk.."
    "...krk..."
    "..krkrk.."
    ".krk.krk."
    "krk...krk"
    "kk.....kk";

static const char BALL[] =
    ".kkkk."
    "krrwwk"
    "kwrrwk"
    "kwwrrk"
    "kwwrrk"
    ".kkkk.";

/* the kite, flying over Burl */
static const char KITE_BIG[] =
    "......k......"
    ".....kPk....."
    "....kPPKk...."
    "...kPPPKKk..."
    "..kPPPPKKKk.."
    ".kPPPPPKKKKk."
    "kkkkkkkkkkkkk"
    ".kKKKKKPPPPk."
    "..kKKKKPPPk.."
    "...kKKKPPk..."
    "....kKKPk...."
    ".....kKk....."
    "......k......";

/* the spinner's blades, two frames side by side */
static const char SPINNER_BIG[] =
    "kkkkkkkkkkkkkk"
    "kyyyyyykyyyyyk"
    ".kkkkkkakkkkk."
    "......kak....."
    ".....kBBBk....";


typedef struct { int id, w, h; const char *data; } ArtDef;

void tkw_art_load(void) {
    if (tkw_spr[SP_BURL].px) return;
    static const ArtDef ART[] = {
        {SP_BURL, 20, 14, BURL}, {SP_BURL_WALK, 20, 14, BURL_WALK}, {SP_BURL_CROUCH, 20, 14, BURL_CROUCH},
        {SP_BURL_FLAP1, 20, 14, BURL_FLAP1}, {SP_BURL_FLAP2, 20, 14, BURL_FLAP2}, {SP_BURL_SLEEP, 20, 14, BURL_SLEEP},
        {SP_BURL_FALL, 20, 14, BURL_FALL},
        {SP_COCKLE, 7, 6, COCKLE}, {SP_WHELK, 9, 9, WHELK}, {SP_KEY, 9, 5, KEY}, {SP_SPRAT, 10, 5, SPRAT},
        {SP_SPIRAL, 7, 7, SPIRAL}, {SP_CHEST, 14, 10, CHEST}, {SP_CHEST_OPEN, 14, 10, CHEST_OPEN},
        {SP_LIGHTHOUSE, 12, 7, LIGHTHOUSE}, {SP_TERN1, 9, 6, TERN1}, {SP_TERN2, 9, 6, TERN2},
        {SP_DUCHESS, 12, 16, DUCHESS}, {SP_BELL, 9, 8, BELL}, {SP_BELL_DONE, 9, 8, BELL_DONE},
        {SP_RAM, 18, 12, RAM}, {SP_RAM_RUN, 18, 12, RAM_RUN}, {SP_MANTA1, 24, 8, MANTA1}, {SP_MANTA2, 24, 8, MANTA2},
        {SP_ELDER, 28, 20, ELDER},
        {SP_I_BOBBER, 9, 9, I_BOBBER}, {SP_I_SPYGLASS, 9, 9, I_SPYGLASS}, {SP_I_GRAPNEL, 9, 9, I_GRAPNEL},
        {SP_I_KITE, 9, 9, I_KITE}, {SP_I_SPINNER, 9, 9, I_SPINNER}, {SP_I_TIN, 9, 9, I_TIN}, {SP_I_CROSS, 9, 9, I_CROSS},
        {SP_BALL, 6, 6, BALL}, {SP_KITE_BIG, 13, 13, KITE_BIG}, {SP_SPINNER_BIG, 14, 5, SPINNER_BIG},
    };
    for (int i = 0; i < ARRAY_LEN(ART); i++) spr_make(&tkw_spr[ART[i].id], ART[i].w, ART[i].h, ART[i].data);
}
