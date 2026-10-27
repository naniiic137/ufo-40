/* HELLSPUR - the story: the burning saloon, every person's words, the
 * gates the story keeps shut and the keys that open them, the boss fights
 * and what they leave behind, each stranger's own scene, Sage's animals,
 * and the ending. All words are ours. */
#include "spur.h"

enum {
    EV_NONE, EV_CARRY, EV_DONT, EV_LEAVE_SALOON, EV_POSSE_TEXT,
    EV_FIGHT_BRUTUS = 10, EV_GIVE_HONEY, EV_FIGHT_TALLY, EV_FIGHT_JACK, EV_FIGHT_MAGPIE, EV_FIGHT_MOLLY, EV_PAY_MOLLY,
    EV_FIGHT_MIMIC, EV_FIGHT_PUCK, EV_FIGHT_SCOURGE, EV_FIGHT_CROW, EV_FIGHT_WORM, EV_FIGHT_MOTHER, EV_FIGHT_BLIGHT,
    EV_FIGHT_CARNAGE, EV_FIGHT_HUNGER, EV_FIGHT_REAPER, EV_FIGHT_GOV, EV_FIGHT_FINAL,
    EV_GIVE_MEDCHEST = 30, EV_DREAM_YES, EV_SLEEP_PAST, EV_SMITH_YES, EV_BUY_HELLITE, EV_BURRO_BUY, EV_WIDOW_REST,
    EV_ENDING, EV_LETTER_BANK, EV_FERRY_FIX, EV_BLAST, EV_PULL_SPIKE, EV_PAST_WAKE, EV_AMBROSE_GONE, EV_REAPER_GIFT,
    EV_FINAL_TALK, EV_TRADE_FEATHER, EV_SIEGE, EV_MOTHER_TRUTH, EV_GOV_FLED, EV_CANCAN, EV_GIL_BELL, EV_MEDCHEST_BUY
};
#define F_MEDCHEST_SOLD (F_MISC0 + 12) /* the gentleman outside the Dolores saloon has sold his one */
enum { PK_ANIMAL = 200 };

static int pending_hero = -1, pending_animal = -1, pending_spike = -1;
static char tb[400];

static bool in_posse(int h) {
    for (int i = 0; i < 4; i++) if (spur_hero_at(i) == h) return true;
    return false;
}
static bool posse_has_alive(int h) { return in_posse(h) && spur_alive(h); }

static void fight(int troop) {
    sg.pending_battle = troop;
    sg.pending_flag = -1;
}

const char *const SPUR_INTRO[] = {
    "THE TERRITORY OF CALICO WAS HARD COUNTRY\nBEFORE IT FELL. NOW IT HANGS IN HELL,\nNAILED THERE BY THREE IRON SPIKES,\nAND THE SUN COMES UP RED.",
    "IN GLADWELL, AT THE GOLDEN GARTER SALOON,\nTHE WIDOW TALLOW SENT FOR SEVEN STRANGERS\nAND A DOG. NOBODY KNEW WHY.",
    "THEY HAD NOT FINISHED THEIR FIRST DRINK\nWHEN THE DOORS WERE BARRED FROM OUTSIDE\nAND THE FIRE CAME UP THROUGH THE BOARDS.",
    "YOU ARE THE ANGEL WHO HEARD THEM.\nYOU HAVE TIME TO CARRY OUT FOUR.\nNO MORE.",
};
const int SPUR_INTRO_PAGES = ARRAY_LEN(SPUR_INTRO);

/* ---- the saloon --------------------------------------------------------------------- */
static void saloon_talk(int slot) {
    int h = slot; /* the strangers stand in hero order */
    const SpurHeroDef *d = &SPUR_HERO[h];
    pending_hero = h;
    if (sg.nsaved >= 4) {
        snprintf(tb, sizeof tb, "%s\n%s\fYOUR ARMS ARE FULL.", d->pick_line[0], d->pick_line[1]);
        spur_say(tb);
        return;
    }
    snprintf(tb, sizeof tb, "%s\n%s\fCARRY OUT %s, THE %s?", d->pick_line[0], d->pick_line[1], d->name, d->klass);
    spur_ask(tb, EV_CARRY, EV_DONT);
}

static void saloon_leave(void);
void spur_saloon_exit(void) { saloon_leave(); }

static void saloon_leave(void) {
    if (sg.nsaved == 0) { spur_say("YOU CANNOT LEAVE THEM ALL TO BURN."); return; }
    char names[96] = "";
    for (int h = 0; h < H_COUNT; h++)
        if (sg.saved[h]) {
            if (names[0]) strcat(names, ", ");
            strcat(names, SPUR_HERO[h].name);
        }
    snprintf(tb, sizeof tb, "LEAVE WITH %s?\nTHE REST WILL BURN.", names);
    spur_ask(tb, EV_LEAVE_SALOON, 0);
}

static void make_posse(void) {
    sg.s.nposse = 0;
    memset(sg.s.order, 0xFF, sizeof sg.s.order);
    for (int h = 0; h < H_COUNT; h++) {
        spur_hero_init(h);
        if (sg.saved[h] && sg.s.nposse < 4) {
            sg.s.hero[h].in = 1;
            sg.s.order[sg.s.nposse++] = (uint8_t)h;
        }
    }
    sg.s.teeth = 200;
    sg.s.chapel = M_GLADWELL;
    spur_set_flag(F_POSSE);
    sg.s.valid = 1;
}

/* ---- people --------------------------------------------------------------------------- */
static void animal_talk(int a) {
    const SpurAnimal *an = &SPUR_ANIMAL[a];
    if (!in_posse(H_SAGE) || !spur_alive(H_SAGE)) {
        static const char *const NOISE[SPUR_ANIMALS] = {"THE JACKRABBIT TWITCHES ITS NOSE.", "THE OWL BLINKS, SLOWLY.",
            "THE KIT FOX GRINS AT YOU.", "THE ROADRUNNER LOOKS AT YOU SIDEWAYS.", "THE ARMADILLO CURLS UP.",
            "THE BURRO FLICKS AN EAR.", "THE HORNED TOAD PUFFS UP.", "THE SCORPION RAISES ITS TAIL.",
            "THE COUGAR WATCHES, AND DOESN'T BLINK.", "THE CONDOR SPREADS WINGS WIDE AS A WAGON."};
        if (a == 2 && posse_has_alive(H_BISCUIT) && !spur_flag(F_LACKEY_GIFT)) {
            spur_set_flag(F_LACKEY_GIFT);
            if (spur_bag_add(IT_WISHBONE) >= 0) { spur_say("THE KIT FOX SNIFFS BISCUIT. THEY HAVE A LONG TALK.\nIT DROPS A WISHBONE AT HIS FEET."); return; }
        }
        spur_say(NOISE[a]);
        return;
    }
    if (spur_flag(F_ANIMAL0 + a)) {
        snprintf(tb, sizeof tb, "THE %s HAS SAID ALL IT HAS TO SAY.", an->name);
        spur_say(tb);
        return;
    }
    pending_animal = a;
    char t[64];
    snprintf(t, sizeof t, "THE %s OFFERS SAGE A GIFT", an->name);
    const char *opts[2] = {SPUR_SKILL[an->a].name, SPUR_SKILL[an->b].name};
    spur_pick(t, opts, 2, PK_ANIMAL);
}

static void boss_talk(int art) {
    switch (art) {
    case A_PUCKKING: spur_ask("A FAT PUCK WEARING A CROWN OF BOTTLE CAPS.\n\"MINE! ALL MINE! THE SHINY HANDLE TOO!\"\fTAKE THE CRANK FROM HIM?", EV_FIGHT_PUCK, 0); break;
    case A_SCOURGE: spur_ask("A JAILER WITH A WHIP FOR EACH HAND, AND\nMORE HANDS THAN WHIPS.\n\"VISITING HOURS ARE OVER. FOREVER.\"\fFIGHT?", EV_FIGHT_SCOURGE, 0); break;
    case A_STORMCROW: spur_ask("THE STORM CROW SITS ON THE PASS LIKE A CLOUD\nWITH A BEAK. THE AIR SMELLS OF LIGHTNING.\fFIGHT?", EV_FIGHT_CROW, 0); break;
    case A_WORM: spur_ask("THE ROCK SPLITS AND A WORM OF CINDERS\nRISES OUT OF THE LAVA, HISSING.\fFIGHT?", EV_FIGHT_WORM, 0); break;
    case A_MOTHER: spur_ask("A SAGUARO TALLER THAN A CHURCH, WITH A FACE.\n\"YOU WANT THE TRUTH? THE TRUTH HAS THORNS.\nSHOW ME YOU CAN HOLD IT.\"\fFIGHT?", EV_FIGHT_MOTHER, 0); break;
    case A_BLIGHT: spur_ask("A RIDER WRAPPED IN FLIES ON A YELLOW HORSE.\n\"SMELL THAT? THAT'S THE OLD WORLD ROTTING.\"\fFIGHT?", EV_FIGHT_BLIGHT, 0); break;
    case A_CARNAGE: spur_ask("A RIDER IN RED IRON ON A RED HORSE.\n\"YOU WON'T TURN BACK. GOOD. NEITHER WILL I.\"\fFIGHT?", EV_FIGHT_CARNAGE, 0); break;
    case A_HUNGER: spur_ask("A RIDER SO THIN THE SADDLE SHOWS THROUGH.\n\"WHEN DID YOU LAST EAT, LITTLE THINGS?\nI CAN HEAR YOUR BELLIES FROM HERE.\"\fFIGHT?", EV_FIGHT_HUNGER, 0); break;
    case A_REAPER: spur_ask("THE LAST RIDER WEARS A PALE HAT AND AN OLD\nTIN STAR. \"I'M TIRED, POSSE. BUT A JOB'S\nA JOB.\"\fFIGHT?", EV_FIGHT_REAPER, 0); break;
    case A_GOVERNOR: spur_ask("THE GOVERNOR OF HELLSPUR SITS ON A THRONE\nOF LEDGERS. HIS SMILE IS TOO WIDE.\n\"THE PAPERWORK SAYS YOU'RE ALREADY DEAD.\"\fFIGHT?", EV_FIGHT_GOV, 0); break;
    default: spur_say("..."); break;
    }
}

static void talk_gladwell(int s) {
    switch (s) {
    case 0: spur_say("THE GOLDEN GARTER BURNED TO ITS BONES.\nTHEY SAY THE DOORS WERE BARRED FROM OUTSIDE.\nWHO'D DO SUCH A THING?"); break;
    case 1:
        if (!spur_flag(F_WREN)) spur_say("AN ANGEL WAS SEEN AT ST. CLARE MISSION,\nUP THE ROAD NORTH. A REAL ONE, WINGS AND ALL.\nIF ANYONE KNOWS WHY THE GARTER BURNED...");
        else spur_say("WE'RE PRAYING FOR YOU. FOR WHAT IT'S WORTH,\nDOWN HERE.");
        break;
    case 2: spur_say("THE BLUE FIRES COME OUT AT YOU IN THE GRASS!\nIF ONE TOUCHES ANY OF YOU, EVEN THE ONE AT\nTHE BACK, IT'S A FIGHT!"); break;
    case 3:
        if (spur_bag_find(IT_HELLITE) >= 0 || spur_flag(F_SMITH_DONE)) { spur_say("SOLD YOU THE ONLY ONE I HAD."); break; }
        spur_ask("GOT A LUMP OF BLACK HELLITE HERE. HOT TO THE\nTOUCH. A SMITH IN HELL WOULD KILL FOR IT.\fYOURS FOR 100,000 BITS. BUY?", EV_BUY_HELLITE, 0);
        break;
    case 4:
        if (posse_has_alive(H_BISCUIT)) spur_say("THE SHERIFF'S HOUND TOUCHES NOSES WITH BISCUIT.\nTHEY AGREE ON SOMETHING.");
        else spur_say("WOOF.");
        break;
    case 5:
        if (spur_flag(F_WON)) spur_say("WE'RE RAISING THE GARTER AGAIN, BOARD BY BOARD.\nTHERE'LL BE A ROOM FOR EACH OF YOU.");
        else spur_say("I'M A CARPENTER. SOMEDAY I'LL BUILD THE GARTER\nBACK. SOMEDAY WHEN IT'S SAFE TO.");
        break;
    case 6: spur_say("SHERIFF. A BANK UP IN MERIDIAN PAYS INTEREST\nAFTER EVERY FIGHT YOU WIN. PUT YOUR BITS IN\nIT BEFORE YOU GO ANYWHERE STUPID."); break;
    }
}

static void talk_meridian(int s) {
    switch (s) {
    case 0: spur_say("MONEY IN THE BANK IS SAFE. IF THE WISPS DRAG\nYOU DOWN, YOU LOSE WHAT'S IN YOUR POCKETS.\nNOT WHAT'S IN THE VAULT."); break;
    case 1: spur_say("HIRED HORSES CAN OUTRUN THE WISPS. THEY STEP\nONCE FOR EVERY TWO OF YOURS."); break;
    case 2: spur_say("THE TRAIN DON'T RUN. HOPE SPRINGS IS FULL OF\nDEVILS AND THEY TORE UP THE LINE."); break;
    case 3:
        if (spur_key(KI_STORMFEATHER) && !spur_flag(F_STORM_TRADED)) {
            spur_ask("THAT'S A STORM CROW FEATHER! I'VE WANTED ONE\nSINCE I WAS A BOY. I'LL TRADE YOU MY WARD\nFOR IT. KEEPS YOUR WITS UNDER A SPELL.\fTRADE?", EV_TRADE_FEATHER, 0);
        } else spur_say("SHERIFF OF MERIDIAN. THE FERRY AT MULE\nCROSSING'S BROKE. WITHOUT IT NOBODY CROSSES\nTHE WIDE RIVER.");
        break;
    case 4:
        if (spur_flag(F_FERRY)) spur_say("YOU FIXED THE FERRY? HAH! THE PUCKS'LL BE\nSORE ABOUT THAT.");
        else spur_say("THE FERRY CRANK? THE PUCKS DRAGGED IT DOWN\nSIXPENNY MINE, NORTH-WEST OF TOWN. PUCKS\nLOVE ANYTHING SHINY.");
        break;
    case 5: spur_say("THE HORSE LOOKS AT YOU LIKE YOU OWE IT MONEY."); break;
    case 6:
        if (in_posse(H_HATTIE) && !spur_flag(F_LETTER_PAID)) {
            if (spur_key(KI_LETTER)) spur_ask("THE TELLER READS THE LETTER TWICE.\n\"YOUR BROTHER AMOS WAS WIRING YOU HIS SAVINGS,\nMISS HATTIE. 3,000 BITS. INTO THE BANK?\"", EV_LETTER_BANK, 0);
            else spur_say("A TELLER: \"HATTIE? THERE WAS A LETTER FOR YOU.\nYOUR BROTHER AMOS RODE OFF WITH IT FOR HOPE\nSPRINGS. NOBODY'S SEEN HIM SINCE.\"");
        } else spur_say("I WORK AT THE BANK. TWO IN A HUNDRED AFTER\nEVERY FIGHT, ROUNDED UP. UP TO 100,000.");
        break;
    case 7: spur_say("I DUG SIXPENNY FOR TWENTY YEARS. IF ANY OF\nYOU KNOWS HOW TO PROSPECT, THE GLINTING SEAMS\nDOWN THERE ARE WORTH A FORTUNE."); break;
    case 8: spur_say("GINGER POP PUTS THE SOUL BACK IN YOU. THE\nSALOON'S GOT IT, A HUNDRED AND TWENTY A BOTTLE."); break;
    }
}

static void talk_stclare(int s) {
    switch (s) {
    case 0: spur_say("WELCOME, TRAVELLERS. THE BEDS HERE ARE FREE\nAND SO IS THE PRAYING."); break;
    case 1:
        if (!spur_flag(F_WREN)) {
            spur_set_flag(F_WREN);
            spur_say("THE ANGEL WHO CARRIED YOU OUT TURNS.\n\"I AM WREN. I'M SORRY I COULD ONLY TAKE FOUR.\"\f\"ANOTHER OF MY KIND, AMBROSE, IS CHAINED IN THE\nDEEP PEN ACROSS THE WIDE RIVER. HE KNOWS WHY\nTHE GARTER BURNED.\"\f\"THE FERRY AT MULE CROSSING IS BROKEN.\nASK IN MERIDIAN. GO, AND FREE HIM.\"");
        } else if (!spur_flag(F_AMBROSE)) spur_say("\"AMBROSE IS IN THE DEEP PEN, NORTH-EAST OF HOPE\nSPRINGS, ACROSS THE RIVER. PLEASE HURRY.\"");
        else spur_say("\"YOU FREED HIM. I CAN FEEL IT. WHATEVER HE\nTELLS YOU... TRY TO FORGIVE HIM.\"");
        break;
    case 2: spur_say("THE PRIEST: \"WE BURY WHO WE CAN. HELL DOESN'T\nKEEP ITS DEAD QUIET.\""); break;
    case 3: spur_say("A CHAPEL REMEMBERS YOU. IF THE WISPS EVER TAKE\nYOU ALL, YOU'LL WAKE IN THE LAST ONE YOU\nSTOPPED IN."); break;
    case 4: spur_say("THERE'S A JACKRABBIT OUT PAST THE MISSION THAT\nTALKS. ONLY SPIRIT FOLK CAN HEAR IT THOUGH."); break;
    }
}

static void talk_hope(int s) {
    bool free = spur_flag(F_HOPE_FREE);
    switch (s) {
    case 0:
        if (spur_key(KI_HONEYCOMB)) spur_ask("BRUTUS FILLS THE DOORWAY OF THE JAIL.\n\"NOBODY SEES THE TALLYMAN.\" HE SNIFFS.\n\"...IS THAT HONEY?\"\fGIVE HIM THE HONEYCOMB?", EV_GIVE_HONEY, EV_FIGHT_BRUTUS);
        else spur_ask("BRUTUS FILLS THE DOORWAY OF THE JAIL, BIG AS\nA BARN DOOR.\n\"NOBODY SEES THE TALLYMAN.\"\fFIGHT HIM?", EV_FIGHT_BRUTUS, 0);
        break;
    case 1: case 2: spur_say("\"MOVE ALONG, MEAT.\""); break;
    case 3:
        if (free) spur_say("THE DEVILS ARE GONE! THE CHAPEL'S OPEN, THE\nSALOON TOO, AND THE TRAIN'S RUNNING AGAIN!");
        else spur_say("THE TALLYMAN TOOK THE JAIL FOR HIS COUNTING\nHOUSE. HE WRITES DOWN EVERY SOUL IN TOWN.");
        break;
    case 4: spur_say(free ? "GOD BLESS YOU." : "STRANGERS DON'T LAST HERE. THE TALLYMAN\nSENDS HIS DEVILS ROUND AT SUPPER."); break;
    case 5: spur_say("BRUTUS HAS A SWEET TOOTH. HE SITS BY THE BEE\nTREES DOWN SOUTH-EAST WHEN HE'S OFF DUTY."); break;
    case 6: spur_say("THE DEEP PEN IS NORTH-EAST, PAST THE CREEK.\nTHE GROUND ROUND IT IS BAD. BIG THINGS WALK\nTHERE THAT SHOULDN'T BE THAT FAR SOUTH."); break;
    case 7: spur_say("\"SALOON'S CLOSED TO YOUR KIND.\""); break;
    case 8: spur_say("WIDOW'S NOTCH IS THE ONLY WAY NORTH, AND\nANGELS STAND ACROSS IT. THEY LET NOBODY BY."); break;
    }
}

static void talk_steeple(int s) {
    switch (s) {
    case 0: spur_say("STEEPLETON'S SALOON IS A CHAPEL NOW. WE WERE\nDRINKING TOO MUCH AND PRAYING TOO LITTLE."); break;
    case 1:
        if (in_posse(H_QUILL) && spur_alive(H_QUILL) && !spur_flag(F_QUILL_SW)) {
            spur_set_flag(F_QUILL_SW);
            sg.s.hero[H_QUILL].passive_bonus[PS_SURVIVOR]++;
            spur_sfx("spur_learn");
            spur_say("\"QUILL. THE BOY WHO WOULDN'T STAY DOWN.\" MAE\nFLINT, WHO BROKE HIS NOSE IN '79, SQUARES UP.\fTHEY TRADE WORDS, THEN NOT-QUITE PUNCHES.\nTHEN THEY LAUGH.\f\"STILL GETTING BACK UP, SCHOLAR?\"\nQUILL LEARNS SECOND WIND.");
        } else spur_say("I USED TO BOX. BEAT A SCHOLAR ONCE, A SKINNY\nTHING. HE KEPT GETTING BACK UP.");
        break;
    case 2: spur_say("THE SHERIFF SELLS A LITTLE BOOK ON KEEPING A\nSTEADY HAND. MAKES THE RED OF THE BAR BIGGER."); break;
    case 3: spur_say("THE SHERIFF'S AT HIS DESK IF YOU WANT HIM."); break;
    case 4: spur_say("THE HORSE IS EATING A HAT."); break;
    case 5:
        if (spur_flag(F_JACK)) spur_say("THE 4:15 RUNS AGAIN, THANKS TO YOU.");
        else spur_say("JACKAL JACK AND HIS PACK HOLD THE STATION.\nNO TRAINS IN OR OUT. GO SEE THE STATIONMASTER.");
        break;
    case 6: spur_say(spur_flag(F_FORT) ? "THE FORT HELD. I HEAR IT WAS YOU." : "FORT PATIENCE IS UNDER SIEGE, WEST OF HERE.\nDEVILS ALL ROUND THE WALLS."); break;
    case 7: spur_say("THE WOODS SOUTH-WEST OF TOWN ARE ONLY BIG\nTHINGS. APES. BUFFALO MEN. GOOD MONEY IF YOU\nLIVE."); break;
    }
}

static void talk_fort(int s) {
    switch (s) {
    case 0:
        spur_say("AMBROSE LEANS ON THE WALL.\n\"THE WAR GOES BADLY. I'M GOING WEST, TO LAST\nCHANCE. THE SMUGGLERS' CUT IS THE ONLY WAY.\"\f\"IT'S BLOCKED WITH ROCK. THE SOLDIERS HAD\nDYNAMITE IN IT, BUT THE PLUNGER WENT MISSING.\nASK IN DOLORES.\"");
        break;
    case 1: spur_say("THANK GOD YOU CAME. THEY'D HAVE HAD US BY\nSUNDOWN."); break;
    case 2: spur_say("SOMEBODY FROM DOLORES STOLE THE PLUNGER FOR\nTHE CUT'S DYNAMITE. OLD GIL KNOWS THE WHOLE\nOUTLAW CROWD."); break;
    case 3: spur_say("THE CHAPLAIN: \"WE LOST GOOD MEN. THE GRAVES\nBEHIND THE BARRACKS ARE ALL WE COULD DIG.\""); break;
    case 4: spur_say("THE CUT IS EAST OF THE SPINE MOUNTAINS, DUE\nWEST OF STEEPLETON. YOU'LL SEE THE ROCKFALL."); break;
    case 5:
        if (in_posse(H_HOLLIS) && spur_alive(H_HOLLIS) && !spur_flag(F_SOLDIER_RAISE)) {
            if (spur_bag_find(IT_MEDCHEST) >= 0) spur_ask("A SOLDIER LIES BLEEDING ON A COT. HOLLIS\nKNEELS. \"I CAN SAVE HIM, WITH A MED CHEST.\"\fGIVE HIM A MED CHEST?", EV_GIVE_MEDCHEST, 0);
            else spur_say("A SOLDIER LIES BLEEDING ON A COT. HOLLIS\nLOOKS HIM OVER. \"WITH A MED CHEST I COULD\nSAVE HIM. WE'D BETTER BE QUICK.\"");
        } else if (spur_flag(F_SOLDIER_RAISE)) spur_say("\"I OWE YOU MY LIFE, DOC. HERE'S HOPING I CAN\nPAY IT BACK.\"");
        else spur_say("THE SOLDIER ON THE COT IS IN A BAD WAY.");
        break;
    case 6: spur_say("THE BEDS IN THE BARRACKS ARE FREE FOR YOU,\nFOR AS LONG AS YOU WANT THEM."); break;
    case 7: spur_say("THERE'S A GRAVE BACK THERE NOBODY PUT UP. IT\nSAYS SOMETHING ABOUT A SAUCER."); break;
    }
}

static void talk_dolores(int s) {
    switch (s) {
    case 0: spur_say("DOLORES AIN'T GOT NO SHERIFF NO MORE. WE GOT\nBLACK MOLLY INSTEAD."); break;
    case 1:
        if (spur_flag(F_MOLLY) && !spur_flag(F_GIL_GAVE)) {
            spur_set_flag(F_GIL_GAVE);
            spur_give_key(KI_BELL);
            spur_say("OLD GIL CLAPS YOUR HANDS. \"MOLLY'S DONE WITH\nME? HERE. MY SILVER BELL. RING IT AT THE\nMISSION OF THORNS. SOMETHING'S WRONG THERE.\"\fYOU GET THE SILVER BELL.");
        } else if (spur_flag(F_GIL_GAVE)) spur_say("\"RING THAT BELL AT THORNS. FALSE THINGS CAN'T\nSTAND THE SOUND OF IT.\"");
        else spur_ask("OLD GIL, BY THE WELL. \"BLACK MOLLY SAYS I OWE\nHER 6,000. SHE'S HOLED UP AT THE OUTLAW ROOST,\nNORTH-EAST.\"\f\"OR YOU COULD BUY MY SILVER BELL. 6,000, AND\nI'LL WALK IT UP TO HER MYSELF.\"\fBUY THE BELL FOR 6,000?", EV_GIL_BELL, 0);
        break;
    case 2: animal_talk(5); break;
    case 3:
        if (spur_flag(F_ANIMAL0 + 5)) { spur_say("\"ENJOY THE MULE.\""); break; }
        spur_ask("\"FINE TAME BURRO, YOURS FOR 1,000 BITS.\nIT TALKS, SOME SAY. TO SPIRIT FOLK.\"\fBUY?", EV_BURRO_BUY, 0);
        break;
    case 4: spur_say("THE PLUNGER? MOLLY'S BOYS HAD IT. THEN THEY\nWENT DIGGING FOR SILVER IN THE HOLLOW GAP\nMINE. NOBODY CAME BACK."); break;
    case 5:
        if (posse_has_alive(H_BISCUIT)) spur_say("THE TOWN DOG LEADS BISCUIT TO THE WELL AND\nWHINES AT IT. HE KNOWS SOMETHING.");
        else spur_say("THE DOG WHINES AT THE WELL.");
        break;
    case 6: spur_say("MOLLY'S ROOST HAS A LOCKED DOOR IN THE BACK.\nGOES DOWN INTO AN OLD MINE. KEY'S LOST."); break;
    case 8: spur_say("A CARD SHARP IN THE CORNER, BACK TO THE WALL.\n\"MAGPIE DEALS FROM THE BOTTOM OF THE DECK.\nEVERYBODY KNOWS IT. NOBODY SAYS IT.\""); break;
    case 9:
        if (spur_flag(F_MEDCHEST_SOLD)) { spur_say("\"SOLD YOU MY ONLY ONE, FRIEND. LOOK AFTER IT.\""); break; }
        spur_ask("A GENTLEMAN IN A CHECKED SUIT, OUTSIDE THE\nSALOON. \"A REAL ARMY MED CHEST. SEALED. 8,000\nBITS AND IT'S YOURS.\"\fBUY IT?", EV_MEDCHEST_BUY, 0);
        break;
    case 7:
        spur_ask("SLICK MAGPIE, THE GAMBLER, DEALS YOU A CARD.\n\"THE SALOON STAGE IS MINE, AND SO'S EVERY\nDANCER ON IT. CARE TO ARGUE?\"\fFIGHT?", EV_FIGHT_MAGPIE, 0);
        break;
    }
}

static void talk_hollow(int s) {
    bool past = spur_flag(F_MISC0);
    switch (s) {
    case 0: spur_say("WELCOME TO HOLLOW GAP! BEST SILVER IN CALICO.\nTHE MINE'S BEEN SINGING LATELY, THOUGH."); break;
    case 1: spur_say("MY HUSBAND SAYS THERE'S SOMETHING DOWN IN THE\nMINE THAT KNOCKS BACK WHEN YOU KNOCK."); break;
    case 2: spur_say("IF THE BAD THINGS EVER COME, I'M HIDING IN THE\nWELL. IT'S MY SECRET. DON'T TELL."); break;
    case 3: spur_say("THE OLD PROSPECTOR, IN THE ASSAY OFFICE:\n\"I'LL TEACH ANY OF YOU TO PROSPECT. A BOOK\nOF IT, 8,000 BITS.\""); break;
    case 4: spur_say("WHAT YEAR IS IT? WHY, IT'S '61, STRANGER."); break;
    case 5:
        if (in_posse(H_TUCKER) && spur_alive(H_TUCKER) && !spur_flag(F_TUCKER_COVER)) {
            spur_set_flag(F_TUCKER_COVER);
            spur_learn_skill(H_TUCKER, SK_COVER);
            spur_sfx("spur_learn");
            spur_say("THE STOREKEEPER LOOKS UP. \"TUCKER? YOU'VE GOT\nOLD.\" IT'S DUTCH, WHO RODE THE DRIVE WITH HIM.\nWHO DIED IN THE MINE FIRE TWENTY YEARS AGO.\f\"YOU ALWAYS STOOD IN FRONT OF THE HERD, YOU\nBIG FOOL. KEEP DOING IT.\"\fTUCKER LEARNS COVER.");
        } else spur_say("THE STOREKEEPER, DUTCH: \"BUY SOMETHING OR\nGIT. I'VE GOT INVENTORY.\"");
        break;
    case 6: spur_say(past ? "WOOF." : "A THIN DOG SITS IN THE EMPTY STREET, WAITING\nFOR SOMEONE."); break;
    }
}

static void talk_lastchance(int s) {
    switch (s) {
    case 0: spur_say("LAST CHANCE. THE LAST TOWN THAT'S STILL A\nTOWN. AFTER THIS IT'S THE GATE AND HELL."); break;
    case 1: spur_say("THE WIDOW TALLOW CAME HERE OFTEN. TALKED ABOUT\nA CACTUS MOTHER WHO KNOWS EVERYTHING. AT THE\nAGAVE SHRINE, SOUTH-EAST OF TOWN."); break;
    case 2:
        if (spur_flag(F_REAPER)) spur_say("CASSIUS BOWS HIS HEAD. \"AMBROSE IS GONE. I\nFELT IT. THANK YOU FOR BEING WITH HIM.\"");
        else spur_say("THE ANGEL CASSIUS: \"AMBROSE? HE CAME THROUGH.\nSAID HE HAD TO MEET SOMEONE IN THE BONE TOWER,\nIN HELL ITSELF. HE DIDN'T LOOK HAPPY ABOUT IT.\"");
        break;
    case 3: spur_say("THE SHERIFF SELLS HALO PINS. ONE CARRIES YOU\nHOME TO THE LAST CHAPEL YOU STOPPED IN."); break;
    case 4: spur_say("THE BLACK GATE IS NORTH OF TOWN. IT'S GOT A\nHEEL CUT IN IT LIKE A BOOT SPUR."); break;
    case 5: spur_say("THE HORSE IS DOING ITS BEST."); break;
    case 6: spur_say("THE AGAVE SHRINE'S SHAMAN WILL GIVE YOU THE\nDREAM WINE. DRINK IT AND YOU WALK IN THE\nDREAMING. NOT EVERYONE COMES BACK."); break;
    case 7: spur_say("I'M AN OUTLAW, BUT EVEN I WON'T GO NEAR THAT\nGATE."); break;
    case 8: spur_say("THEY SAY HELL'S BONE TOWER HAS FOUR FLOORS AND\nA RIDER ON EACH."); break;
    }
}

static void talk_ashmarket(int s) {
    switch (s) {
    case 0: spur_say("A DEMON IN A WAISTCOAT: \"HUMANS! AND A DOG.\nBUSINESS IS BUSINESS, COME IN, COME IN.\""); break;
    case 1: spur_say("\"THREE IRON SPIKES PIN THE TERRITORY TO THE\nFLOOR OF HELL. PULL THEM AND THE VEIL ROUND\nHELLSPUR LIFTS. EVERYONE KNOWS THAT.\""); break;
    case 2: spur_say("\"ONE SPIKE'S ON A MESA TOP IN THE NORTH-EAST.\nONE'S IN A LAKE IN THE BURNT HILLS. ONE'S OUT\nON A ROCK IN THE EASTERN SEA.\""); break;
    case 3: spur_say("\"YOU'LL WANT SOMETHING TO PULL THEM WITH. THE\nHERMITS IN THE HIDDEN VALLEY SOUTH OF THE\nBOTTOMS HAVE A CLAW BAR, I HEAR.\""); break;
    case 4: spur_say("\"HELLSPUR'S IN THE CRATER, FAR NORTH-WEST.\nTHE GOVERNOR KEEPS THE BOOKS THERE.\""); break;
    case 5: spur_say("\"THERE'S A SMITH OF OURS WEST OF GLADWELL, IN\nA PIT. GIVE HER HELLITE AND SHE'LL MAKE YOU A\nRIFLE TO END ARGUMENTS.\""); break;
    case 6: spur_say("\"SOMETHING BIG RUNS ON IRON RAILS OUT ON THE\nSOUTHERN BLUFF. THE JUGGERNAUT. SMART FOLK\nDON'T LAND THERE.\""); break;
    }
}

static void talk_hermitage(int s) {
    switch (s) {
    case 0: spur_say("A SISTER: \"WE BREED DOGS HERE. BIG ONES. OUR\nBONES MAKE A HOUND STRONGER. THE KENNEL SELLS\nTHEM.\""); break;
    case 1:
        if (!spur_flag(F_CLAWBAR)) {
            spur_set_flag(F_CLAWBAR);
            spur_give_key(KI_CLAWBAR);
            spur_say("THE ABBESS: \"ZENITH AND NADIR. ONE IN HEAVEN,\nONE IN HELL, AND BOTH IN HELLSPUR, PLAYING\nCARDS WITH SOULS FOR CHIPS.\"\f\"PULL THE THREE SPIKES THAT PIN CALICO DOWN,\nAND THE VEIL ROUND HELLSPUR WILL FALL. TAKE\nOUR CLAW BAR. AND END IT.\"\fYOU GET THE CLAW BAR.");
        } else spur_say("\"THREE SPIKES. THEN HELLSPUR. GO WITH WHATEVER\nGOD IS LEFT.\"");
        break;
    case 2: spur_say(posse_has_alive(H_BISCUIT) ? "THE BIG DOG AND BISCUIT RACE ROUND THE POND." : "THE BIG DOG WANTS TO PLAY."); break;
    case 3: spur_say("A HERMIT: \"WE CAME HERE BEFORE THE FALL. NOW\nONLY WINGS CAN FIND US. HOW DID YOU... OH.\nTHE NIGHTMARES. OF COURSE.\""); break;
    case 4: spur_say("THE BEDS IN THE CHAPEL ARE FREE."); break;
    }
}

static void talk_place(int map, int s) {
    switch (map) {
    case M_PIKESHACK:
        if (s == 0) {
            if (!spur_flag(F_PIKE_PAID)) {
                spur_set_flag(F_PIKE_PAID);
                sg.s.teeth += 500;
                spur_sfx("spur_buy");
                spur_say("EZRA PIKE PRESSES A PURSE ON YOU. \"500 BITS.\nIT'S WHAT HE PAID ME. I DON'T WANT IT.\"");
            } else spur_say("\"I'M SORRY. I'LL BE SORRY TILL I DIE.\"");
        } else spur_say(spur_flag(F_PIKE_FOUND) ? "\"YOU BROUGHT MY BROTHER HOME. WHATEVER HE DID.\"" : "\"MY BROTHER EZRA RAN OFF SOUTH-EAST, TO THE DRY\nWASH CAVE. HE WAS CRYING ABOUT A FIRE.\"");
        break;
    case M_ROOST:
        if (s == 0) {
            if (spur_flag(F_MOLLY)) {
                if (spur_flag(F_MOLLY_PAID)) { spur_open_door(SV_SHOP_A, 15); break; }
                spur_say("BLACK MOLLY SCOWLS. \"GIL'S SQUARE. NOW GET\nOUT OF MY ROOST.\"");
                break;
            }
            if (in_posse(H_VERA) && spur_alive(H_VERA))
                spur_ask("BLACK MOLLY SPITS. \"VERA. STILL ALIVE. STILL\nWEARING THE FACE OFF THE POSTERS?\"\f\"GIL OWES ME 6,000. FOR YOU, OLD FRIEND...\n4,000, AND I'LL SELL YOU MY BLACK BREW AFTER.\"\fPAY 4,000 FOR GIL?", EV_PAY_MOLLY, 0);
            else spur_ask("BLACK MOLLY PUTS DOWN HER GLASS. \"GIL SENT\nYOU? THEN YOU OWE ME 6,000 TOO.\"\fFIGHT?", EV_FIGHT_MOLLY, 0);
        } else if (s == 1) spur_say("\"MOLLY'S GOT A POT OF BLACK BREW HID SOMEWHERE.\nTHAT STUFF WAKES THE DEAD SOUL.\"");
        else if (s == 2) spur_say("\"THAT DOOR GOES DOWN TO THE OLD MINE. THE KEY\nWENT TO THE MISSION FOLK, YEARS BACK.\"");
        else spur_say("\"DON'T LOOK AT ME. I JUST WORK HERE.\"");
        break;
    case M_THORNS:
        if (s == 0) {
            if (spur_key(KI_BELL)) spur_ask("THE PRIEST SMILES AND SMILES. \"BLESS YOU.\nBLESS YOU. BLESS YOU.\"\fRING THE SILVER BELL?", EV_FIGHT_MIMIC, 0);
            else spur_say("THE PRIEST SMILES. \"BLESS YOU, CHILD.\" HIS EYES\nDON'T MOVE WHEN HIS HEAD DOES.");
        } else if (s == 1) spur_say(spur_flag(F_MIMIC) ? "\"THE STAIRS WERE BEHIND HIM ALL ALONG!\"" : "A NUN WHISPERS: \"THAT IS NOT FATHER LUIS.\nFATHER LUIS HAD TWO EYEBROWS.\"");
        else if (s == 2) spur_say("THE COLLECTION PLATE IS FULL OF BITS.");
        else if (s == 3) spur_say("I CAME TO PRAY. I DON'T REMEMBER WHY I STAY.");
        else spur_say("THE KID IS COUNTING THE PEWS. HE KEEPS GETTING\nA DIFFERENT NUMBER.");
        break;
    case M_WIDOW:
        if (s == 0) {
            if (!spur_flag(F_FIRECHARM)) {
                spur_set_flag(F_FIRECHARM);
                spur_give_key(KI_FIRECHARM);
                spur_say("THE WIDOW'S SISTER: \"SHE WROTE THAT SEVEN\nWOULD COME. AND THE DOG. SHE MEANT TO TELL YOU\nWHAT THE ANGELS HAD DONE.\"\f\"SHE LEFT THIS FOR YOU. A FIRE CHARM. THE DEEP\nTUNNELS WEST ARE FULL OF FIRE.\"\fYOU GET THE FIRE CHARM.");
            } else spur_say("\"REST HERE AS LONG AS YOU LIKE. THE BED'S HERS,\nBUT SHE WON'T MIND.\"");
        } else if (s == 1) {
            if (posse_has_alive(H_BISCUIT)) spur_say("THE NIECE HUGS BISCUIT. \"YOU WERE AUNT\nTALLOW'S DOG! YOU CAME HOME!\" BISCUIT LOOKS\nAT THE DOOR, WAITING FOR SOMEONE.");
            else spur_say("\"AUNT TALLOW HAD A DOG, EAR ALL NOTCHED. HE WENT WITH\nHER TO GLADWELL.\"");
        } else spur_say("THE NIECE'S BROTHER IS FEEDING A LIZARD.");
        break;
    case M_SHRINE:
        if (s == 0) {
            if (spur_flag(F_MOTHER) && !spur_key(KI_IRONSPUR)) {
                spur_give_key(KI_IRONSPUR);
                spur_say("THE SHAMAN HANDS YOU A HEAVY IRON SPUR.\n\"THE MOTHER SAYS YOU ARE READY. THIS FITS THE\nHEEL OF THE BLACK GATE. GO NORTH.\"\fYOU GET THE IRON SPUR.");
            } else if (spur_flag(F_MOTHER)) spur_say("\"THE GATE IS NORTH OF LAST CHANCE.\"");
            else if (!spur_flag(F_MISC0 + 1)) spur_ask("THE SHAMAN: \"YOU WANT TO MEET THE MOTHER?\nDRINK THE AGAVE WINE AND WALK THE DREAMING.\"\fDRINK?", EV_DREAM_YES, 0);
            else spur_say("\"THE WAY INTO THE DREAMING IS OPEN. WALK IT.\"");
        } else if (s == 1) spur_say("THE WIDOW TALLOW CAME TO THE MOTHER EVERY\nSPRING. SHE KNEW SOMETHING ABOUT THE ANGELS.");
        else spur_say("THE DREAMING HAS ITS OWN PLANTS. GLINTING ONES.");
        break;
    case M_SKYBAR:
        if (s == 0) spur_open_door(SV_SHOP_A, 14);
        else if (s == 1) spur_say("AN ANGEL, OFF DUTY: \"YOU FOUND THE SKY BAR!\nDON'T TELL ZENITH. WE'RE SUPPOSED TO BE\nWATCHING THE WAR.\"");
        else spur_say("\"THE BLACK BREW HERE NEVER RUNS DRY.\"");
        break;
    case M_SMITH:
        if (spur_flag(F_SMITH_DONE)) { spur_say("THE SMITH WIPES HER HANDS. \"ONE RIFLE. THERE\nWILL NEVER BE ANOTHER.\""); break; }
        if (spur_bag_find(IT_HELLITE) >= 0)
            spur_ask("THE DEMON SMITH SEES THE HELLITE AND HER EYES\nCATCH FIRE. \"HELLITE! GIVE ME THAT AND I'LL\nMAKE YOU THE DOOMSDAY RIFLE.\"\fDEAL?", EV_SMITH_YES, 0);
        else spur_say("THE DEMON SMITH: \"I ONLY WORK IN HELLITE.\nBRING ME SOME. DIG IT, OR BUY IT.\"");
        break;
    case M_SPUR3:
        if (s <= 1) spur_ask("ZENITH AND NADIR LOOK UP FROM THEIR CARDS.\n\"THE MORTALS WON THE HAND,\" SAYS NADIR.\n\"THEN WE'LL DEAL AGAIN,\" SAYS ZENITH.\f\"THE ANGELS AND THE DEVILS FIGHT, THE SOULS\nPAY THE BETS, AND WE ALWAYS WIN. YOU'RE JUST\nCHIPS, LITTLE POSSE.\"\fEND THE GAME?", EV_FIGHT_FINAL, 0);
        else spur_say(s == 2 ? "AN ANGEL GUARD: \"WE WERE NEVER FIGHTING FOR\nHEAVEN. ONLY FOR ZENITH'S SIDE OF THE TABLE.\""
                             : "A DEVIL GUARD: \"KILL ZENITH AND THE GAME'S\nOVER. NADIR CAN'T PLAY ALONE.\"");
        break;
    }
}

static void talk_dungeon(int map, int s) {
    const SpurNpc *n = &sg.map.npc[s];
    if (n->look == LOOK_ANIMAL) { animal_talk(n->pal); return; }
    if (n->look == LOOK_BOSS) { boss_talk(n->pal); return; }
    switch (map) {
    case M_DRYWASH:
        spur_set_flag(F_PIKE_FOUND);
        spur_say("A MAN HUDDLES IN THE DARK. \"I'M EZRA PIKE.\nI SET THE FIRE AT THE GARTER. A MAN WITH WINGS\nPAID ME. I DIDN'T KNOW THERE WERE FOLK INSIDE.\"\f\"I'M GOING HOME TO MY BROTHER, WEST OF\nGLADWELL. FIND ME THERE. I'LL MAKE IT RIGHT.\"\fHE RUNS PAST YOU INTO THE LIGHT.");
        break;
    case M_PEN1:
        if (in_posse(H_HATTIE) && !spur_flag(F_LETTER)) {
            spur_set_flag(F_LETTER);
            spur_give_key(KI_LETTER);
            spur_say("A DEAD MAN IN A CELL. HATTIE GOES WHITE.\n\"AMOS.\" HER BROTHER. IN HIS HAND, A SEALED\nLETTER FROM THE MERIDIAN BANK, FOR HER.\fYOU GET THE LETTER.");
        } else spur_say("A DEAD MAN IN A CELL, STILL WAITING FOR HIS\nVISITORS.");
        break;
    case M_PEN2:
        if (!spur_flag(F_AMBROSE)) {
            spur_set_flag(F_AMBROSE);
            spur_bag_add(IT_HALOPIN);
            spur_say("THE ANGEL AMBROSE SHAKES OFF HIS CHAINS.\n\"YOU CAME. THEN I OWE YOU THE TRUTH: I PAID\nEZRA PIKE TO BURN THE GARTER.\"\f\"I HAD MY REASONS. I'M NOT SURE NOW THEY WERE\nGOOD ONES. THE WIDOW KNEW TOO MUCH, AND YOU\nWERE ALL... WITNESSES.\"\f\"THE ANGELS AT WIDOW'S NOTCH WILL LET YOU BY.\nI'M GOING TO FORT PATIENCE. TAKE THIS HALO PIN\nTO GET HOME.\"\fHE IS GONE IN A RUSH OF WINGS.");
            spur_refresh_map();
        } else spur_say("...");
        break;
    case M_DREAM1:
        if (in_posse(H_SAGE) && spur_alive(H_SAGE) && !spur_flag(F_SAGE_BEADS)) {
            spur_set_flag(F_SAGE_BEADS);
            if (spur_bag_add(IT_BEADS) < 0) { spur_clear_flag(F_SAGE_BEADS); spur_say("\"MAKE ROOM IN YOUR BAGS, GIRL. THEN COME BACK.\""); break; }
            spur_say("AN OLD WOMAN SITS BY A FIRE THAT GIVES NO\nSMOKE. SAGE STOPS DEAD. \"GRANDMOTHER?\"\f\"YOU ALWAYS DID WALK TOO FAR FROM HOME.\" SHE\nPRESSES A STRING OF BEADS INTO SAGE'S HANDS.\n\"FOR WHEN YOUR SPIRIT RUNS DRY.\"\fYOU GET THE SPIRIT BEADS.");
        } else spur_say("AN OLD WOMAN BY A SMOKELESS FIRE NODS AT YOU.\n\"KEEP WALKING. THE MOTHER IS DEEPER IN.\"");
        break;
    case M_TOWER4:
        spur_say("AMBROSE LIES BLEEDING BEHIND THE THRONE.\n\"I CAME TO MEET THE REAPER, TO BUY MY WAY OUT.\nZENITH SENT ME TO DO SUCH THINGS... SO MANY.\"\f\"THE WIDOW TALLOW... WAS SHE AS BAD AS THEY\nSAID? I NEVER ASKED.\" HIS EYES CLOSE.\fTHERE'S A BULLET IN HIS BACK. ANGEL-MADE.");
        break;
    default: spur_say("..."); break;
    }
}

/* folk passing the time indoors */
static void inside_chat(int service, int k) {
    static const char *const HOTEL[] = {
        "\"THE CLERK WRITES EVERY GUEST IN HIS BOOK.\nIF THE WISPS GET YOU, AT LEAST YOUR NAME'S KEPT.\"",
        "\"A NIGHT'S SLEEP MENDS THE LIVING. THE DEAD\nWANT A CHAPEL.\"",
        "\"UPSTAIRS IS QUIETER. FOLK LEAVE THINGS\nUNDER THE BEDS.\"",
        "\"I SLEEP WITH MY BOOTS ON. THE WISPS DON'T\nKNOCK.\"",
    };
    static const char *const SALOON[] = {
        "\"GINGER POP TASTES LIKE A STRUCK MATCH. PUTS\nTHE SPARK BACK IN YOUR SOUL.\"",
        "\"A WORD WITH THE BARKEEP'S WORTH MORE THAN\nTHE DRINK. HE HEARS EVERYTHING.\"",
        "\"I'D SING, BUT THE PIANO'S BEEN SHOT TWICE\nTHIS WEEK.\"",
        "\"EVER SEE A BARKEEP COME OUT FROM BEHIND HIS\nBAR? SOME BARS HAVE A LOOSE FLAP, I HEAR.\"",
    };
    static const char *const SHOP[] = {
        "\"TWO LITTLE PISTOLS FIRE TWICE AS OFTEN AS\nONE BIG ONE. IF YOU'VE THE HANDS FOR IT.\"",
        "\"THEY'LL BUY ORE HERE. THE SHINIER THE\nBETTER.\"",
        "\"TWENTY THINGS IN THE BAGS AND NOT ONE MORE.\nSORT THEM NOW AND THEN.\"",
        "\"SNAKEROOT FOR THE POISON. NEVER LEAVE TOWN\nWITHOUT IT.\"",
    };
    static const char *const BANK[] = {
        "\"THE VAULT KEEPS WHAT THE WISPS CAN'T TAKE.\"",
        "\"TWO IN A HUNDRED AFTER EVERY FIGHT YOU WIN.\nIT ADDS UP.\"",
        "\"I KEEP MY SAVINGS IN MY BOOT. THE TELLER\nLAUGHS AT ME.\"",
        "\"EVERY BANK IN CALICO KNOWS YOUR BALANCE.\nDON'T ASK ME HOW.\"",
    };
    static const char *const LAW[] = {
        "\"THE CELLS ARE EMPTY. NOBODY STAYS LOCKED UP\nIN HELL FOR LONG.\"",
        "\"THE DEPUTY PLAYS CARDS WITH HIMSELF. AND\nLOSES.\"",
        "\"IF YOU SEE THE SHERIFF, TELL HIM HIS COFFEE'S\nGONE COLD.\"",
    };
    static const char *const CHAPEL[] = {
        "\"THE CHAPEL REMEMBERS YOU. FALL OUT THERE AND\nYOU WAKE UP IN THESE PEWS.\"",
        "\"PRAYER HELPS. A PRIEST WITH A PRICE HELPS\nMORE.\"",
        "\"WE LIGHT A CANDLE FOR EVERY STRANGER WHO\nPASSES. WE'RE OUT OF CANDLES.\"",
    };
    int ti = spur_town_here() - SPUR_FIRST_TOWN;
    switch (service) {
    case SV_HOTEL: spur_say(HOTEL[(ti + k) % 4]); break;
    case SV_SALOON: spur_say(SALOON[(ti + k * 3) % 4]); break;
    case SV_SHOP_A: case SV_SHOP_B: spur_say(SHOP[(ti + k + service) % 4]); break;
    case SV_BANK: spur_say(BANK[(ti + k) % 4]); break;
    case SV_SHERIFF: case SV_STORY: spur_say(LAW[(ti + k) % 3]); break;
    case SV_CHAPEL: spur_say(CHAPEL[(ti + k) % 3]); break;
    default: spur_say("..."); break;
    }
}

void spur_talk(int npc) {
    const SpurNpc *n = &sg.map.npc[npc];
    int map = sg.map_id;
    int s = n->talk % 32;
    if (map == M_INSIDE) {
        /* the counter, the folk, or the town's own people indoors */
        if (n->talk >= 0xF000) { spur_open_door(n->talk & 0xFF, sg.inside.arg); return; }
        if (n->talk >= 0xE000) { inside_chat((n->talk >> 4) & 0xFF, n->talk & 15); return; }
        map = n->talk / 32;
    }
    if (n->look == LOOK_ANIMAL) { animal_talk(n->pal); return; }
    if (n->look == LOOK_BOSS && sg.map.kind == MK_DUNGEON) { boss_talk(n->pal); return; }
    switch (map) {
    case M_SALOON: saloon_talk(s); return;
    case M_GLADWELL: talk_gladwell(s); return;
    case M_MERIDIAN: talk_meridian(s); return;
    case M_STCLARE: talk_stclare(s); return;
    case M_HOPE: talk_hope(s); return;
    case M_STEEPLE: talk_steeple(s); return;
    case M_FORT: talk_fort(s); return;
    case M_DOLORES: talk_dolores(s); return;
    case M_HOLLOW: talk_hollow(s); return;
    case M_LASTCHANCE: talk_lastchance(s); return;
    case M_ASHMARKET: talk_ashmarket(s); return;
    case M_HERMITAGE: talk_hermitage(s); return;
    default: break;
    }
    if (sg.map.kind == MK_DUNGEON) talk_dungeon(map, npc);
    else talk_place(map, s);
}

/* ---- doors the story answers ----------------------------------------------------------- */
/* doors the story keeps shut: you stay in the street */
bool spur_story_gate(int service) {
    int map = spur_town_here();
    if (map == M_HOPE && !spur_flag(F_HOPE_FREE) && (service == SV_CHAPEL || service == SV_SALOON || service == SV_TRAIN)) {
        spur_say("A DEVIL BLOCKS THE DOOR. \"CLOSED. BY ORDER\nOF THE TALLYMAN.\"");
        return true;
    }
    return false;
}

bool spur_story_door(int service, int arg) {
    int map = spur_town_here();
    (void)arg;
    if (map == M_HOPE && !spur_flag(F_HOPE_FREE)) {
        if (service == SV_STORY) { spur_ask("THE TALLYMAN SITS AT A DESK OF LEDGERS, FOUR\nIMPS AT HIS ELBOWS. \"MORE SOULS FOR THE BOOK.\"\fFIGHT?", EV_FIGHT_TALLY, 0); return true; }
        if (service == SV_CHAPEL || service == SV_SALOON || service == SV_TRAIN) { spur_say("A DEVIL BLOCKS THE DOOR. \"CLOSED. BY ORDER\nOF THE TALLYMAN.\""); return true; }
    }
    if (map == M_HOPE && service == SV_STORY) { spur_say("THE JAIL IS EMPTY NOW, EXCEPT FOR THE LEDGERS."); return true; }
    if (map == M_STEEPLE && service == SV_TRAIN && !spur_flag(F_JACK)) {
        spur_ask("THE STATIONMASTER WRINGS HIS HANDS. \"JACKAL\nJACK'S IN THE CAR, WITH HIS WHOLE PACK.\"\fGO ABOARD AND FIGHT HIM?", EV_FIGHT_JACK, 0);
        return true;
    }
    if (map == M_STEEPLE && service == SV_SHERIFF) { spur_open_door(SV_SHOP_B, 17); return true; }
    if (map == M_LASTCHANCE && service == SV_SHERIFF) {
        /* the sheriff's desk sells halo pins */
        sg.ui.kind = UI_NONE;
        return false;
    }
    if (map == M_MERIDIAN && service == SV_SHERIFF) { talk_meridian(3); return true; }
    if (map == M_MERIDIAN && service == SV_BANK && spur_key(KI_LETTER) && !spur_flag(F_LETTER_PAID) && in_posse(H_HATTIE)) { talk_meridian(6); return true; }
    if (map == M_DOLORES && service == SV_SALOON && in_posse(H_CORA) && spur_alive(H_CORA) && spur_flag(F_MAGPIE) && !spur_flag(F_CANCAN)) {
        spur_set_flag(F_CANCAN);
        spur_learn_skill(H_CORA, SK_CANCAN);
        spur_sfx("spur_learn");
        spur_say("WITH MAGPIE GONE, THE DANCERS PULL CORA UP ON\nTHE STAGE. THE PIANO GOES WILD. SO DOES CORA.\fCORA LEARNS THE CANCAN.");
        return true;
    }
    if (map == M_DOLORES && service == SV_SALOON && !spur_flag(F_MAGPIE)) { spur_say("THE BARKEEP NODS AT THE STAGE. \"MAGPIE'S\nHOUSE, MAGPIE'S RULES. NO POURS TILL HE SAYS.\""); return true; }
    if (map == M_HOLLOW) {
        bool past = spur_flag(F_MISC0);
        if (service == SV_HOTEL && !past) { spur_ask("THE HOTEL IS A RUIN, BUT SOME OF THE BEDS\nARE WHOLE. SLEEP HERE TONIGHT?", EV_SLEEP_PAST, 0); return true; }
        if (service == SV_HOTEL && past) { spur_ask("A NICE ROOM, FREE FOR TRAVELLERS. SLEEP?", EV_PAST_WAKE, 0); return true; }
        if ((service == SV_SHOP_A || service == SV_SHOP_B) && !past) { spur_say("NOTHING BUT DUST AND A COUNTER WITH NOBODY\nBEHIND IT."); return true; }
    }
    if (map == M_ASHMARKET && service == SV_SALOON) { spur_say("THE DEMON BARKEEP: \"WE DON'T SERVE YOUR KIND.\nNOT EVEN IN HELL.\" HE POLISHES A GLASS. \"BUT\nTHE SPIKES. YOU'RE AFTER THE SPIKES, AREN'T YOU?\""); return true; }
    if (map == M_LASTCHANCE && service == SV_SHERIFF) return false;
    return false;
}

/* ---- tiles you search ------------------------------------------------------------------- */
void spur_open_chest(int i) {
    const SpurChest *c = &sg.map.chest[i];
    char m[160];
    if (c->item >= 1000) {
        int k = c->item - 1000;
        spur_give_key(k);
        spur_set_chest(c->id);
        snprintf(m, sizeof m, "INSIDE: THE %s.", SPUR_KEYITEM_NAME[k]);
        if (k == KI_CRANK) spur_set_flag(F_PUCKKING);
        if (k == KI_CELLARKEY) spur_set_flag(F_CRYPT_KEY);
        if (k == KI_PLUNGER) spur_set_flag(F_PLUNGER);
        spur_sfx("spur_key");
        spur_say(m);
        return;
    }
    if (c->item == -1) { spur_set_chest(c->id); return; }
    if (c->item > 0) {
        if (spur_bag_add(c->item) < 0) { spur_say("THERE'S SOMETHING HERE, BUT THE BAGS ARE FULL."); return; }
        snprintf(m, sizeof m, "%s %s.", c->hidden ? "TUCKED AWAY:" : "INSIDE:", SPUR_ITEM[c->item].name);
    } else if (c->teeth == SPUR_CHEST_EMPTY) {
        snprintf(m, sizeof m, "NOTHING. NOT EVEN DUST.");
    } else if (c->teeth == SPUR_CHEST_JOKE) {
        snprintf(m, sizeof m, "INSIDE: 100,000 BITS!\f...PAINTED ON THE BOTTOM OF THE CHEST.\nSOMEBODY DOWN HERE HAS A SENSE OF HUMOUR.");
    } else {
        sg.s.teeth += c->teeth;
        snprintf(m, sizeof m, "%s %d BITS.", c->hidden ? "TUCKED AWAY:" : "INSIDE:", c->teeth);
    }
    spur_set_chest(c->id);
    spur_sfx("spur_chest");
    spur_say(m);
}

void spur_search_tile(int x, int y, int t) {
    /* a hidden find first */
    for (int i = 0; i < sg.map.nchest; i++) {
        const SpurChest *c = &sg.map.chest[i];
        if (c->hidden && c->x == x && c->y == y && !spur_chest_open(c->id) && c->item != -1) { spur_open_chest(i); return; }
    }
    switch (t) {
    case T_FERRY:
        if (spur_flag(F_FERRY)) return;
        if (spur_key(KI_CRANK)) { spur_ask("THE FERRY'S CRANK SOCKET IS EMPTY. FIT THE\nFERRY CRANK?", EV_FERRY_FIX, 0); return; }
        spur_say("THE FERRY CABLE HANGS SLACK ACROSS THE WIDE\nRIVER. THE CRANK THAT WINDS IT IS GONE.");
        return;
    case T_ANGELS: spur_say("ANGELS STAND ACROSS WIDOW'S NOTCH, SWORDS\nDRAWN. \"NONE PASS WITHOUT AMBROSE'S WORD.\""); return;
    case T_RUBBLE:
        if (spur_key(KI_PLUNGER)) { spur_ask("A ROCKFALL BLOCKS THE SMUGGLERS' CUT. A FUSE\nRUNS INTO IT FROM AN OLD CHARGE.\fFIT THE PLUNGER AND BLOW IT?", EV_BLAST, 0); return; }
        spur_say("A ROCKFALL BLOCKS THE SMUGGLERS' CUT. A FUSE\nRUNS INTO THE ROCK. ALL IT NEEDS IS A PLUNGER.");
        return;
    case T_FIREWALL: spur_say("A WALL OF FIRE FILLS THE TUNNEL."); return;
    case T_LOCKED:
        if (spur_key(KI_CELLARKEY)) {
            spur_set_flag(F_ROOST_DOOR);
            spur_take_key(KI_CELLARKEY);
            spur_refresh_map();
            spur_sfx("spur_door");
            spur_say("THE CELLAR KEY TURNS. THE DOOR OPENS ON STAIRS\nDOWN INTO AN OLD MINE.");
        } else spur_say("A HEAVY DOOR, LOCKED TIGHT.");
        return;
    case T_BARRIER: spur_say("A BLACK VEIL HANGS ROUND THE CITADEL. NOTHING\nPASSES IT, NOT EVEN WINGS."); return;
    case T_SPIKE: {
        int which = -1;
        static const int8_t AT[3][2] = {{104, 7}, {33, 30}, {118, 70}};
        for (int i = 0; i < 3; i++) if (AT[i][0] == x && AT[i][1] == y) which = i;
        if (which < 0) return;
        if (!spur_key(KI_CLAWBAR)) { spur_say("A RAILROAD SPIKE AS TALL AS A CHURCH, DRIVEN\nINTO THE GROUND. YOU CAN'T BUDGE IT."); return; }
        pending_spike = which;
        spur_ask("A GIANT IRON SPIKE, NAILING CALICO TO HELL.\nHOOK THE CLAW BAR ON IT AND PULL?", EV_PULL_SPIKE, 0);
        return;
    }
    case T_HIVE:
        if (!spur_flag(F_HIVE) || (!spur_key(KI_HONEYCOMB) && !spur_flag(F_BRUTUS_GONE))) {
            spur_set_flag(F_HIVE);
            spur_give_key(KI_HONEYCOMB);
            spur_sfx("spur_key");
            spur_say("BEES! YOU GRAB A COMB OF WILD HONEY AND RUN.\fYOU GET THE HONEYCOMB.");
        } else spur_say("THE BEES ARE STILL ANGRY ABOUT LAST TIME.");
        return;
    case T_WINCH: {
        /* three a floor (flags 4-6 below, 7-9 above); each winch is its own, counted from the top-left */
        int base = sg.map_id == M_SPUR1 ? 4 : 7;
        int idx = base, nw = 0;
        for (int yy = 0; yy < sg.map.h; yy++)
            for (int xx = 0; xx < sg.map.w; xx++)
                if (sg.map.tile[yy][xx] == T_WINCH) {
                    if (xx == x && yy == y) idx = base + imin(nw, 2);
                    nw++;
                }
        if (spur_flag(F_MISC0 + idx)) { spur_say("THE CHAIN IS ALREADY WOUND IN."); return; }
        spur_set_flag(F_MISC0 + idx);
        spur_sfx("spur_door");
        int pulled = 0;
        for (int i = 4; i < 10; i++) pulled += spur_flag(F_MISC0 + i);
        snprintf(tb, sizeof tb, "YOU WIND THE WINCH. SOMEWHERE A COUNTERWEIGHT\nDROPS. (%d OF 6)%s", pulled,
                 pulled == 6 ? "\fFAR BELOW, BY THE GATE, A LIFT RISES." : "");
        spur_say(tb);
        return;
    }
    case T_GRAVE:
        if (sg.map_id == M_FORT) spur_say("HERE LIES THE FORTIETH BOX.\nTHE SAUCER HAD ROOM FOR FIFTY.");
        else spur_say("A GRAVE. THE NAME HAS WORN AWAY.");
        return;
    case T_BOOKS:
        if (sg.map_id == M_WIDOW) spur_say("THE WIDOW'S DIARY: \"SEVEN WILL COME, AND THE\nDOG. I WILL TELL THEM WHAT AMBROSE DID FOR\nZENITH, AND WHY. THEY MUST KNOW.\"");
        else spur_say("OLD BOOKS. HYMNS AND HORSE DOCTORING.");
        return;
    case T_BED:
        if (sg.map_id == M_WIDOW) { spur_ask("THE WIDOW'S BED. REST HERE?", EV_WIDOW_REST, 0); return; }
        spur_say("A BED. SOMEONE ELSE'S.");
        return;
    case T_PIANO: spur_say("PLINK."); return;
    case T_WELL:
        if (sg.map_id == M_HOLLOW && !spur_flag(F_MISC0)) { spur_say("DOWN THE WELL, A LITTLE TIN SOLDIER ON A LEDGE.\nWHOEVER HID HERE IS LONG GONE."); return; }
        spur_say("A WELL. THE WATER TASTES OF IRON.");
        return;
    case T_ALTAR: spur_say("AN ALTAR, COLD."); return;
    case T_CAULDRON: spur_say("THE FORGE BREATHES OUT HEAT LIKE A DRAGON."); return;
    case T_SIGN: spur_say("THE SIGN IS TOO BURNT TO READ."); return;
    case T_COUNTER: case T_TABLE: return;
    case T_ORE: spur_say("A SEAM GLINTS IN THE ROCK. SOMEONE WHO KNOWS\nHOW TO PROSPECT COULD DIG HERE."); return;
    case T_HERB: return;
    default: spur_say("NOTHING HERE."); return;
    }
}

/* ---- walking into places ------------------------------------------------------------------ */
bool spur_event_door(int map) {
    if (map == M_HELL && !spur_flag(F_GATE_OPEN)) {
        if (spur_key(KI_IRONSPUR)) {
            spur_set_flag(F_GATE_OPEN);
            spur_say("YOU SET THE IRON SPUR IN THE GATE'S HEEL. THE\nBLACK GATE GRINDS OPEN ON A RED WIND.");
            sg.pending_event = 0;
            return true;
        }
        spur_say("A BLACK GATE IN THE ROCK, TALL AS A MESA.\nIN ITS HEEL, A NOTCH THE SHAPE OF A SPUR.");
        return false;
    }
    return true;
}

void spur_enter_event(int map) {
    if (map == M_INSIDE) return;
    int st = SPUR_MAPINFO[map].station;
    if (st) sg.s.stations |= (uint8_t)(1u << (st - 1));
    if (map != M_HOLLOW && spur_flag(F_MISC0)) spur_clear_flag(F_MISC0); /* the past is behind you */
    if (map == M_SKYBAR) spur_set_flag(F_SKYBAR);
    if (map == M_FORT && !spur_flag(F_FORT)) sg.pending_event = EV_SIEGE;
    if (sg.map.kind == MK_TOWN || map == M_STCLARE) {
        /* a chapel town remembers you */
        int ti = map - SPUR_FIRST_TOWN;
        if (ti >= 0 && ti < SPUR_TOWNS && SPUR_TOWN[ti].chapel && !(map == M_HOPE && !spur_flag(F_HOPE_FREE))) sg.s.chapel = (uint8_t)map;
    }
}

void spur_step_event(int x, int y) {
    (void)x;
    (void)y;
}

/* ---- after a boss --------------------------------------------------------------------------- */
bool spur_battle_won_event(int troop) {
    switch (troop) {
    case 82: spur_set_flag(F_PUCKKING); spur_refresh_map(); spur_say("THE PUCK KING SQUEALS AND BURSTS INTO BOTTLE\nCAPS. BEHIND HIM, A CHEST."); return true;
    case 83: spur_set_flag(F_SCOURGE); spur_refresh_map(); spur_say("THE SCOURGE COMES APART AT THE SEAMS. A CELL\nDOOR SWINGS OPEN BEYOND."); return true;
    case 84: spur_set_flag(F_BRUTUS_GONE); spur_set_flag(F_BRUTUS_BEATEN); spur_refresh_map(); spur_say("BRUTUS GOES DOWN LIKE A FELLED OAK, AND\nDOESN'T GET UP. THE JAIL DOOR IS CLEAR."); return true;
    case 85:
        spur_set_flag(F_HOPE_FREE);
        spur_refresh_map();
        spur_say("THE TALLYMAN'S LEDGERS BURN. THE DEVILS RUN\nFROM HOPE SPRINGS HOWLING.\fTHE CHAPEL, THE SALOON AND THE STATION ARE\nOPEN AGAIN.");
        return true;
    case 86: spur_set_flag(F_JACK); spur_say("JACKAL JACK LIMPS OFF INTO THE DESERT. THE\n4:15 BLOWS ITS WHISTLE."); return true;
    case 87: spur_set_flag(F_MAGPIE); spur_refresh_map(); spur_say("SLICK MAGPIE FOLDS. \"THE STAGE IS YOURS.\nTAKE IT.\""); return true;
    case 88: spur_set_flag(F_MIMIC); spur_refresh_map(); spur_say("THE MIMIC SLUMPS INTO A PILE OF EMPTY SKIN.\nBEHIND THE ALTAR, STAIRS GO DOWN."); return true;
    case 89: spur_set_flag(F_MOLLY); spur_refresh_map(); spur_say("BLACK MOLLY SPITS A TOOTH. \"FINE! GIL'S\nSQUARE. GET OUT OF MY ROOST.\""); return true;
    case 90: spur_set_flag(F_FORT); spur_refresh_map(); spur_say("THE PIT CAPTAIN FALLS AND HIS TROOPERS\nSCATTER. THE GATES OF FORT PATIENCE OPEN."); return true;
    case 91:
        spur_set_flag(F_STORMCROW);
        spur_give_key(KI_STORMFEATHER);
        if (!spur_flag(F_SOLDIER_RAISE)) spur_set_flag(F_SOLDIER_DEAD); /* too late for the soldier now */
        spur_refresh_map();
        spur_say("THE STORM CROW BURSTS INTO RAIN. A BLUE\nFEATHER DRIFTS DOWN, CRACKLING.\fYOU GET THE STORM FEATHER.");
        return true;
    case 92:
        spur_set_flag(F_WORM);
        sg.s.stations |= (uint8_t)(1u << (SPUR_MAPINFO[M_LASTCHANCE].station - 1)); /* the train runs there now */
        spur_refresh_map(); spur_say("THE CINDER WORM COOLS TO STONE. THE WAY TO\nLAST CHANCE IS OPEN."); return true;
    case 93:
        spur_set_flag(F_MOTHER);
        spur_refresh_map();
        spur_say("THE SAGUARO MOTHER SINKS, LAUGHING.\n\"GOOD. THEN HEAR IT.\"\f\"ZENITH IN HEAVEN AND NADIR IN HELL ARE NOT\nENEMIES. THEY ARE PARTNERS AT A CARD TABLE.\nYOUR WAR IS THEIR GAME. YOUR SOULS ARE CHIPS.\"\f\"THE WIDOW FOUND OUT. SO ZENITH'S ANGEL BURNED\nHER, AND YOU WITH HER. GO BACK TO THE SHAMAN.\nTHEN GO THROUGH THE BLACK GATE.\"");
        return true;
    case 94: spur_set_flag(F_BLIGHT); spur_refresh_map(); spur_say("BLIGHT DISSOLVES INTO A CLOUD OF FLIES.\nTHE STAIRS UP ARE CLEAR."); return true;
    case 95: spur_set_flag(F_CARNAGE); spur_refresh_map(); spur_say("CARNAGE DROPS HIS SWORD. \"GOOD FIGHT.\"\nHE MEANS IT."); return true;
    case 96: spur_set_flag(F_HUNGER); spur_refresh_map(); spur_say("HUNGER SITS DOWN, LICKING HIS LIPS.\n\"SAVE ME A PLATE.\""); return true;
    case 97:
        spur_set_flag(F_REAPER);
        spur_give_key(KI_NIGHTMARES);
        spur_refresh_map();
        spur_say("THE REAPER TAKES OFF HIS HAT. \"THAT'S IT.\nI QUIT. TOO OLD FOR THIS.\"\f\"TAKE THE HORSES. ALL FOUR. THEY'VE GOT WINGS,\nAND THEY'RE SICK OF ME. GO SETTLE YOUR SCORE.\"\fYOU GET THE NIGHTMARES. MOUNT FROM THE MENU,\nOUT IN THE OPEN.");
        return true;
    case 98: spur_set_flag(F_JUGGERNAUT); spur_say("THE JUGGERNAUT JUMPS ITS RAILS AND TUMBLES\nINTO THE SEA. THE BLUFF GOES QUIET."); return true;
    case 99: spur_set_flag(F_GOVERNOR); spur_refresh_map(); spur_say("THE GOVERNOR'S HIDE SPLITS DOWN THE MIDDLE.\nSOMETHING CRAWLS OUT AND FLEES UP THE STAIRS.\fUP ABOVE, VOICES. AND THE SHUFFLE OF CARDS."); return true;
    case 100:
        spur_set_flag(F_WON);
        game_award(GOAL_SAUCER);
        spur_records.wins++;
        if (sg.cherry_run) { game_award(GOAL_ALIEN); spur_records.cherries++; }
        if (!spur_records.best_battles || sg.s.battles < spur_records.best_battles) spur_records.best_battles = sg.s.battles;
        spur_write_file();
        sg.state = GS_ENDING;
        sg.end_t = 0;
        music_play(SPUR_MUS_ENDING);
        return true;
    default: break;
    }
    return false;
}

/* ---- events --------------------------------------------------------------------------------- */
void spur_event(int ev) {
    if (ev >= 9000) { spur_menu_event(ev); return; }
    switch (ev) {
    case EV_CARRY:
        if (pending_hero >= 0 && sg.nsaved < 4) {
            sg.saved[pending_hero] = 1;
            sg.nsaved++;
            spur_sfx("spur_wings");
            snprintf(tb, sizeof tb, "YOU CARRY %s OUT INTO THE NIGHT, AND COME\nBACK FOR MORE.%s", SPUR_HERO[pending_hero].name,
                     sg.nsaved == 4 ? "\fYOUR ARMS ARE FULL. THE ROOF IS GOING." : "");
            spur_say_then(tb, sg.nsaved == 4 ? EV_LEAVE_SALOON + 100 : 0);
        }
        return;
    case EV_LEAVE_SALOON + 100: saloon_leave(); return;
    case EV_DONT: return;
    case EV_LEAVE_SALOON:
        make_posse();
        spur_say_then("THE GOLDEN GARTER COMES DOWN IN A ROAR OF\nSPARKS. WHOEVER WAS LEFT INSIDE IS GONE.\fTHE ANGEL SETS YOU DOWN ON THE CHAPEL STEPS IN\nGLADWELL, AND THEN SHE IS GONE TOO.", EV_POSSE_TEXT);
        return;
    case EV_POSSE_TEXT:
        spur_enter_map(M_GLADWELL, -1, 0, 1);
        for (int i = 0; i < sg.map.ndoor; i++)
            if (sg.map.door[i].service == SV_CHAPEL) spur_place_party(sg.map.door[i].x, sg.map.door[i].y + 1, 0);
        spur_save_game();
        return;
    case EV_FIGHT_BRUTUS: fight(84); return;
    case EV_GIVE_HONEY:
        spur_take_key(KI_HONEYCOMB);
        spur_set_flag(F_BRUTUS_GONE);
        spur_set_flag(F_HONEY_GIVEN);
        spur_refresh_map();
        spur_say("BRUTUS SNATCHES THE HONEYCOMB AND WADDLES OFF\nTOWARD THE BEE TREES, HUMMING.");
        return;
    case EV_FIGHT_TALLY: fight(85); return;
    case EV_FIGHT_JACK: fight(86); return;
    case EV_FIGHT_MAGPIE: fight(87); return;
    case EV_FIGHT_MIMIC: spur_sfx("spur_bell"); fight(88); return;
    case EV_FIGHT_MOLLY: fight(89); return;
    case EV_PAY_MOLLY:
        if (sg.s.teeth < 4000) { spur_say("\"4,000, VERA. COME BACK WHEN YOU'VE GOT IT.\""); return; }
        sg.s.teeth -= 4000;
        spur_set_flag(F_MOLLY);
        spur_set_flag(F_MOLLY_PAID);
        spur_say("MOLLY COUNTS IT TWICE. \"GIL'S SQUARE. AND MY\nBLACK BREW IS YOURS, A THOUSAND A POT, ANY TIME\nYOU RIDE BY.\"");
        return;
    case EV_FIGHT_PUCK: fight(82); return;
    case EV_FIGHT_SCOURGE: fight(83); return;
    case EV_FIGHT_CROW: fight(91); return;
    case EV_FIGHT_WORM: fight(92); return;
    case EV_FIGHT_MOTHER: fight(93); return;
    case EV_FIGHT_BLIGHT: fight(94); return;
    case EV_FIGHT_CARNAGE: fight(95); return;
    case EV_FIGHT_HUNGER: fight(96); return;
    case EV_FIGHT_REAPER: fight(97); return;
    case EV_FIGHT_GOV: fight(99); return;
    case EV_FIGHT_FINAL: fight(100); return;
    case EV_SIEGE:
        spur_say_then("DEVILS SWARM THE WALLS OF FORT PATIENCE. THEIR\nCAPTAIN TURNS TO FACE YOU.", EV_SIEGE + 100);
        return;
    case EV_SIEGE + 100: fight(90); return;
    case EV_GIVE_MEDCHEST: {
        int s = spur_bag_find(IT_MEDCHEST);
        if (s < 0) return;
        spur_bag_remove(s);
        spur_set_flag(F_SOLDIER_RAISE);
        spur_learn_skill(H_HOLLIS, SK_RAISE);
        spur_sfx("spur_learn");
        spur_say("HOLLIS WORKS ONE-HANDED FOR AN HOUR. THE\nSOLDIER BREATHES EASIER, THEN OPENS HIS EYES.\fSOMETHING IN HOLLIS SETTLES.\nHOLLIS LEARNS RAISE.");
        return;
    }
    case EV_DREAM_YES:
        spur_set_flag(F_MISC0 + 1);
        spur_refresh_map();
        spur_sfx("spur_warp");
        spur_say("THE WINE TASTES OF SMOKE AND SUGAR. THE FLOOR\nOF THE SHRINE OPENS LIKE AN EYE.");
        return;
    case EV_SLEEP_PAST:
        spur_set_flag(F_MISC0);
        spur_refresh_map();
        for (int i = 0; i < 4; i++) {
            int h = spur_hero_at(i);
            if (spur_alive(h)) { sg.s.hero[h].hp = (int16_t)spur_maxhp(h); sg.s.hero[h].sp = (int16_t)spur_maxsp(h); sg.s.hero[h].status = 0; }
        }
        music_play(SPUR_MUS_TOWN);
        spur_say("YOU WAKE TO NOISE IN THE STREET. WAGONS. A\nPIANO. THE HOTEL IS CLEAN AND WHOLE.\fHOLLOW GAP IS ALIVE AGAIN. OR YOU ARE\nSOMEWHERE IT STILL WAS.");
        return;
    case EV_PAST_WAKE:
        spur_clear_flag(F_MISC0);
        for (int i = 0; i < 4; i++) {
            int h = spur_hero_at(i);
            if (spur_alive(h)) { sg.s.hero[h].hp = (int16_t)spur_maxhp(h); sg.s.hero[h].sp = (int16_t)spur_maxsp(h); sg.s.hero[h].status = 0; }
        }
        spur_refresh_map();
        music_play(SPUR_MUS_GHOST);
        spur_say("YOU WAKE IN A RUIN. THE PEOPLE YOU MET LAST\nNIGHT DIED IN '61, WHEN THE MINE OPENED ON HELL.\nTHE DEMONS CAME UP THE NORTH-WEST SHAFT.");
        return;
    case EV_SMITH_YES: {
        int s = spur_bag_find(IT_HELLITE);
        if (s < 0) return;
        sg.s.bag[s] = IT_DOOMSDAY;
        spur_set_flag(F_SMITH_DONE);
        spur_sfx("spur_learn");
        spur_say("THE SMITH HAMMERS FOR A NIGHT AND A DAY.\fYOU GET THE DOOMSDAY RIFLE.");
        return;
    }
    case EV_BUY_HELLITE:
        if (sg.s.teeth < 100000) { spur_say("\"COME BACK WHEN YOU'RE RICH.\""); return; }
        if (spur_bag_add(IT_HELLITE) < 0) { spur_say("NO ROOM IN THE BAGS."); return; }
        sg.s.teeth -= 100000;
        spur_say("THE OLD MAN HANDS IT OVER WITH TONGS.");
        return;
    case EV_GIL_BELL:
        if (sg.s.teeth < 6000) { spur_say("\"6,000, FRIEND. MOLLY COUNTS TWICE.\""); return; }
        sg.s.teeth -= 6000;
        spur_set_flag(F_GIL_GAVE);
        spur_set_flag(F_MOLLY);
        spur_give_key(KI_BELL);
        spur_refresh_map();
        spur_say("GIL TAKES THE BITS UP TO THE ROOST HIMSELF\nAND COMES BACK WHISTLING. \"SQUARE!\"\f\"RING THAT BELL AT THE MISSION OF THORNS.\nSOMETHING'S WRONG THERE.\"\fYOU GET THE SILVER BELL.");
        return;
    case EV_MEDCHEST_BUY:
        if (sg.s.teeth < 8000) { spur_say("\"8,000. I DON'T HAGGLE.\""); return; }
        if (spur_bag_add(IT_MEDCHEST) < 0) { spur_say("NO ROOM IN THE BAGS."); return; }
        sg.s.teeth -= 8000;
        spur_set_flag(F_MEDCHEST_SOLD);
        spur_say("YOU GET A MED CHEST.");
        return;
    case EV_BURRO_BUY:
        if (sg.s.teeth < 1000) { spur_say("\"NO BITS, NO BURRO.\""); return; }
        sg.s.teeth -= 1000;
        if (in_posse(H_SAGE) && spur_alive(H_SAGE)) { spur_say("THE BURRO FOLLOWS SAGE ROUND, TALKING."); animal_talk(5); }
        else spur_say("THE BURRO WANDERS OFF. YOU HAVE BEEN HAD.");
        return;
    case EV_WIDOW_REST:
        for (int i = 0; i < 4; i++) {
            int h = spur_hero_at(i);
            if (spur_alive(h)) { sg.s.hero[h].hp = (int16_t)spur_maxhp(h); sg.s.hero[h].sp = (int16_t)spur_maxsp(h); sg.s.hero[h].status = 0; }
        }
        spur_save_game();
        spur_say("YOU SLEEP WELL. THE GAME IS SAVED.");
        return;
    case EV_LETTER_BANK:
        spur_take_key(KI_LETTER);
        spur_set_flag(F_LETTER_PAID);
        sg.s.bank += 3000; /* even past the cap: a gift is a gift */
        spur_sfx("spur_buy");
        spur_say("3,000 BITS GO INTO THE BANK. HATTIE HOLDS HER\nHANDBAG TIGHTER THAN EVER.");
        return;
    case EV_FERRY_FIX:
        spur_set_flag(F_FERRY);
        spur_take_key(KI_CRANK);
        spur_sfx("spur_door");
        spur_say("YOU FIT THE CRANK AND WIND THE FERRY ACROSS.\nTHE WAY EAST IS OPEN.");
        return;
    case EV_BLAST:
        spur_set_flag(F_CUT_OPEN);
        spur_take_key(KI_PLUNGER);
        spur_refresh_map();
        spur_sfx("spur_boom");
        spur_say("BOOM. THE ROCKFALL COMES DOWN IN A CLOUD OF\nDUST. THE SMUGGLERS' CUT IS OPEN.");
        return;
    case EV_PULL_SPIKE:
        if (pending_spike >= 0) {
            spur_set_flag(F_SPIKE1 + pending_spike);
            spur_refresh_map();
            spur_sfx("spur_boom");
            int n = spur_flag(F_SPIKE1) + spur_flag(F_SPIKE2) + spur_flag(F_SPIKE3);
            snprintf(tb, sizeof tb, "THE GROUND SCREAMS AS THE SPIKE COMES FREE.\n(%d OF 3)%s", n,
                     n == 3 ? "\fFAR TO THE NORTH-WEST, THE VEIL ROUND HELLSPUR\nTEARS LIKE WET PAPER." : "");
            spur_say(tb);
        }
        return;
    case EV_TRADE_FEATHER:
        if (spur_bag_add(IT_WARD) < 0) { spur_say("NO ROOM IN THE BAGS."); return; }
        spur_take_key(KI_STORMFEATHER);
        spur_set_flag(F_STORM_TRADED);
        spur_say("YOU GET THE WARD.");
        return;
    default: break;
    }
}

void spur_story_pick(int mode, int choice) {
    if (mode == PK_ANIMAL) {
        if (choice < 0 || pending_animal < 0) return;
        const SpurAnimal *an = &SPUR_ANIMAL[pending_animal];
        int sk = choice == 0 ? an->a : an->b;
        spur_learn_skill(H_SAGE, sk);
        spur_set_flag(F_ANIMAL0 + pending_animal);
        spur_sfx("spur_learn");
        snprintf(tb, sizeof tb, "THE %s SHOWS SAGE THE WAY OF IT.\nSAGE LEARNS %s.", an->name, SPUR_SKILL[sk].name);
        spur_say(tb);
        return;
    }
    if (mode >= 100 && mode < 100 + M_COUNT) {
        /* a word with the barkeep */
        switch (mode - 100) {
        case M_MERIDIAN: spur_say("\"THEY SAY THE WIDOW TALLOW HAD A SISTER OUT IN\nTHE BURNT HILLS. LIVES ON AN ISLAND IN A LAKE.\""); break;
        case M_HOPE: spur_say("\"THE PEN IS FULL OF THE TALLYMAN'S PRISONERS.\nAND WORSE.\""); break;
        case M_DOLORES: spur_say("\"MAGPIE CHEATS AT CARDS AND AT DANCING.\""); break;
        case M_LASTCHANCE: spur_say("\"A CLOUD SITS ON THE SOUTH RANGE THAT NEVER\nBLOWS AWAY. SOME SAY THERE'S A BAR ON IT. IF\nYOU COULD FLY, I GUESS.\""); break;
        default: spur_say("\"WHAT'LL IT BE?\""); break;
        }
        return;
    }
}

void spur_story_number(int mode, int value) { (void)mode; (void)value; }

/* ---- the ending --------------------------------------------------------------------------- */
static const char *const EPILOGUE[H_COUNT] = {
    "TUCKER TOOK A HERD NORTH, AND NEVER LOST A HEAD.",
    "SAGE WENT BACK TO THE DREAMING, NOW AND THEN, FOR TEA.",
    "HOLLIS OPENED A SURGERY IN GLADWELL. ONE ARM WAS ENOUGH.",
    "QUILL WROTE IT ALL DOWN. NOBODY BELIEVED A WORD.",
    "VERA'S POSTER CAME DOWN. SHE PUT IT ON HER WALL.",
    "HATTIE BOUGHT THE BANK.",
    "CORA DANCED AT THE NEW GARTER EVERY SATURDAY.",
    "BISCUIT SLEPT ON THE WIDOW'S GRAVE, AND THEN ON EVERYONE'S BED.",
};

static int ending_lines(const char **out) {
    static char lead[96], second[96];
    int n = 0;
    int h0 = spur_hero_at(0), h1 = spur_hero_at(1);
    out[n++] = "THE CARDS FLY UP LIKE ASH.";
    out[n++] = "ZENITH AND NADIR ARE GONE, AND THE TABLE WITH THEM.";
    out[n++] = "";
    if (h0 == H_BISCUIT) snprintf(lead, sizeof lead, "BISCUIT BARKS ONCE, LONG AND PROUD.");
    else snprintf(lead, sizeof lead, "%s: \"NOBODY PLAYS CARDS WITH US AGAIN.\"", h0 >= 0 ? SPUR_HERO[h0].name : "");
    out[n++] = lead;
    if (h1 >= 0) {
        if (h0 == H_BISCUIT) snprintf(second, sizeof second, "%s: \"HE SAYS WE OUGHT TO GIT. HE'S RIGHT.\"", SPUR_HERO[h1].name);
        else snprintf(second, sizeof second, "%s: \"THE CITADEL'S COMING DOWN. RUN!\"", SPUR_HERO[h1].name);
        out[n++] = second;
    }
    out[n++] = "";
    out[n++] = "HELLSPUR FELL INTO ITSELF. THE SPIKES ROTTED AWAY,";
    out[n++] = "AND ONE MORNING THE SUN CAME UP YELLOW.";
    out[n++] = "";
    out[n++] = "IN GLADWELL THEY RAISED THE GOLDEN GARTER AGAIN,";
    out[n++] = "WITH A ROOM FOR EACH OF THE POSSE.";
    out[n++] = "";
    for (int i = 0; i < 4; i++) {
        int h = spur_hero_at(i);
        if (h >= 0) out[n++] = EPILOGUE[h];
    }
    out[n++] = "";
    out[n++] = sg.cherry_run ? "EVERY ONE THE ANGEL SAVED CAME HOME." : "NOT EVERY ONE THE ANGEL SAVED CAME HOME.";
    out[n++] = "";
    out[n++] = "HELLSPUR";
    out[n++] = "BEAMDOWN SOFTWORKS, 1988";
    out[n++] = "";
    out[n++] = "THE END";
    return n;
}

const char *spur_ending_line(int k) {
    static const char *lines[48];
    int n = ending_lines(lines);
    return k >= 0 && k < n ? lines[k] : NULL;
}

const char *spur_objective(void) {
    if (!spur_flag(F_WREN)) return "FIND THE ANGEL AT ST. CLARE MISSION";
    if (!spur_flag(F_FERRY)) return "GET THE FERRY CRANK BACK FROM SIXPENNY MINE";
    if (!spur_flag(F_AMBROSE)) return "FREE AMBROSE FROM THE DEEP PEN";
    if (!spur_flag(F_FORT)) return "CROSS WIDOW'S NOTCH. LIFT THE SIEGE OF FORT PATIENCE";
    if (!spur_flag(F_CUT_OPEN)) return "FIND THE PLUNGER FOR THE SMUGGLERS' CUT";
    if (!spur_flag(F_WORM)) return "GET THROUGH THE CUT AND THE TUNNELS TO LAST CHANCE";
    if (!spur_flag(F_MOTHER)) return "MEET THE SAGUARO MOTHER IN THE DREAMING";
    if (!spur_flag(F_REAPER)) return "CLIMB THE BONE TOWER";
    if (!(spur_flag(F_SPIKE1) && spur_flag(F_SPIKE2) && spur_flag(F_SPIKE3))) return "PULL THE THREE IRON SPIKES";
    return "END THE GAME IN HELLSPUR";
}
