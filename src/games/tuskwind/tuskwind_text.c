/* TUSKWIND - every word in the dream: the 22 signs, the hidden one, the
 * story, the old one's two talks and the credits. All written for UFO 40. */
#include "tuskwind.h"

/* The signs, in the order they stand along the way. Someone far ahead is
 * carving them for Burl, one board at a time: a few practical tips for a
 * walrus learning to fly in a dream, a ship that keeps getting nearer, and
 * a voice that thins out towards the end. */
const char *const TKW_SIGN_TEXT[TKW_SIGNS] = {
    "SNOW-NOSE! YOU'RE DREAMING. SO HOLD A, AIM, AND LET GO.",
    "UP HERE A WALRUS CAN FLY, A BIT. KEEP A DOWN IN THE AIR AND PADDLE.",
    "SPRATS PUT THE PUFF BACK IN YOUR FLIPPERS. GULP EVERY ONE.",
    "GULLS SAY A SHIP IS OUT PAST THE BERGS. SMALL, FOR NOW.",
    "A TERN SLEEPS IN EVERY LIGHTHOUSE. WAKE IT, AND IT WILL FISH YOU OUT LATER.",
    "COCKLES, WHELKS... EVERY SHELL YOU POCKET IS A WORD I CAN SEND.",
    "THE SHIP AGAIN. IT HAS SAWS ON DECK. WHO TAKES SAWS FISHING?",
    "SHELLS LIKE TO HIDE: UP UNDER THE CLOUDS, DOWN BY THE WAVES.",
    "IVORY. THAT'S WHAT THE SAWS ARE FOR. AND YOU, LITTLE ONE, ARE MOSTLY IVORY.",
    "A RAM WON'T SHARE ITS ROCK. SKIP OVER ITS HORNS AND LET IT SWIM.",
    "CARVING BOARDS FROM HERE IS SLOW, DEAR. I SPLINTER MORE THAN I SPELL.",
    "THE FLAT FISH GLIDING UP HIGH HAVE KEYS IN THEIR BELLIES. POP ONE.",
    "A BELL ON A POLE CAN HUSH A GALE... OR START ONE. MIND WHICH.",
    "SEA CHESTS TURN UP THEIR LIDS AT WISHES. BRING A KEY.",
    "THE SUN IS SLIDING INTO THE SEA. SAVE A LITTLE PUFF FOR THE DARK.",
    "I CAN COUNT LANTERNS ON THE SHIP NOW. NINE. TEN.",
    "FIFTY SHELLS, BURL. WITH FIFTY I CAN TELL YOU ALL OF IT.",
    "OUT HERE THE STORM WIND NEVER TIRES. FIND A BELL, OR LEAN INTO IT.",
    "MY VOICE IS WEARING THIN AS SPRING ICE.",
    "STEPS, AND A DOOR. COME DOWN TO ME.",
    "...BURL? ...ARE YOU... STILL...",
    "...NEARLY... NEARLY...",
};

/* behind the bar in the top-left corner, for whoever rides a manta there */
const char *const TKW_SECRET_TEXT = "ANYONE WHO RODE A MANTA THIS HIGH HAS EARNED A NAP. TAKE TWO.";

const char *const TKW_INTRO[] = {
    "THE AFTERNOON SUN HAD WARMED ONE PERFECT PATCH OF THE FLOE, AND BURL WAS ASLEEP ON IT.",
    "HE DREAMED OF ISLETS HANGING IN THE SKY, AND OF A DOOR A VERY LONG WAY OFF.",
    "SOMEBODY BEHIND THAT DOOR KEPT SAYING HIS NAME.",
    NULL,
};

/* fewer than 50 shells: Skerry can only shout a little */
const char *const TKW_TALK_GOLD[] = {
    "THERE YOU ARE, SNOW-NOSE! NO, DON'T SETTLE. THERE'S NO TIME.",
    "SHELLS ARE HOW MY VOICE GETS DOWN TO YOU, AND %d IS A THIN ROPE TO SHOUT ALONG.",
    "SO, PLAINLY: A SHIP IS COMING ROUND THE HEADLAND. ITS CREW COLLECTS IVORY.",
    "THE HERD IS ASLEEP ON THE FLOE, AND EVERY ONE OF THEM IS WEARING IVORY.",
    "WHEN YOUR EYES OPEN, ROLL STRAIGHT OFF THE ICE AND SWIM. TAKE ALL THE OTHERS WITH YOU.",
    "ROLL, SNOW-NOSE! SWIM!",
    NULL,
};

/* fifty or more: time enough for all of it */
const char *const TKW_TALK_CHERRY[] = {
    "AH, PLENTY OF SHELLS. NOW I CAN HEAR MYSELF. HELLO, BURL.",
    "THEY CALL ME SKERRY, WHEN ANYONE REMEMBERS. I WAS OLD WHEN YOUR FLOE WAS A SNOWFLAKE.",
    "I'VE BEEN TAPPING ON THE HERD'S DREAMS FOR MORE TIDES THAN ANYONE COUNTED. YOU'RE THE FIRST TO TAP BACK.",
    "THE BAD NEWS: A SHIP IS COMING ROUND THE HEADLAND, FULL OF MEN WHO COLLECT IVORY.",
    "THE GOOD NEWS: YOU NEEDN'T RUN. THE OCEAN AND I ARE OLD FRIENDS.",
    "SO WAKE UP, CLIMB TO THE HIGHEST BIT OF THE FLOE, AND LOOK THAT SHIP IN THE EYE.",
    "THEN SLAP THE ICE WITH YOUR FLIPPER, HARD. ONCE. TWICE. THRICE.",
    "THE OCEAN WILL KNOW WHO'S ASKING. OFF YOU GO, SNOW-NOSE.",
    NULL,
};

const char *const TKW_CREDITS[] = {
    "TUSKWIND",
    "",
    "A BEAMDOWN SOFTWORKS DREAM",
    "1986",
    "",
    "BURL ........ HIMSELF",
    "SKERRY ...... THE OLD ONE",
    "DUCHESS AUK . THE STALLS",
    "THE TERNS ... THE RESCUES",
    "THE RAMS .... THE BUMPS",
    "",
    "MADE FOR THE UFO 40",
    "THANK YOU FOR DREAMING",
    NULL,
};

static int count(const char *const *pages) {
    int n = 0;
    while (pages[n]) n++;
    return n;
}

int tkw_talk_pages(bool cherry) { return count(cherry ? TKW_TALK_CHERRY : TKW_TALK_GOLD); }

const char *tkw_talk_page(bool cherry, int i, char *buf, int n) {
    const char *const *pages = cherry ? TKW_TALK_CHERRY : TKW_TALK_GOLD;
    if (i < 0 || i >= tkw_talk_pages(cherry)) return "";
    if (strstr(pages[i], "%d")) {
        snprintf(buf, (size_t)n, pages[i], tkg.shells);
        return buf;
    }
    return pages[i];
}
