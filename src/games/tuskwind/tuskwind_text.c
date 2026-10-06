/* TUSKWIND - every word in the dream: the 22 signs, the hidden one, the
 * story, the old one's two talks and the credits. All written for UFO 40. */
#include "tuskwind.h"

/* the signs, in the order they stand along the way */
const char *const TKW_SIGN_TEXT[TKW_SIGNS] = {
    "SOMETHING SITS LOW ON THE SEA, FAR OFF.",
    "A SHADOW ON THE WATER. IT IS NOT A WHALE.",
    "LOOK HARD AT THE EDGE OF THE SKY. THERE.",
    "THEY WANT YOUR TUSKS. EVERY LAST TUSK.",
    "DO YOU SEE IT YET, LITTLE ONE?",
    "THERE IS STILL TIME. COME FURTHER.",
    "HEAR ME. PLEASE. KEEP LISTENING.",
    "DON'T WAKE. NOT YET. NOT QUITE YET.",
    "SIGNPOSTS. WHAT A SLOW WAY TO SPEAK.",
    "I WAS STRONGER ONCE. THE YEARS ARE LONG.",
    "THE WAY IS WHAT MATTERS NOW. ON YOU GO!",
    "EVERY SHELL YOU CARRY BRINGS MY VOICE NEARER.",
    "DON'T STOP. THIS MUST REACH YOU.",
    "THE MANTAS HOLD A GIFT. BURST ONE AND SEE.",
    "YOU DON'T KNOW WHO I AM. NOT YET.",
    "THE SEA HERSELF COULD HELP, IF SHE CARED TO.",
    "BRING ME SHELLS AND I CAN SAY MORE.",
    "FIFTY SHELLS. REMEMBER THAT: FIFTY.",
    "A LEAP IN THE DARK NEVER HURT A DREAMER.",
    "IT IS ONLY A DREAM, AFTER ALL...",
    "THE LINE GROWS THIN... I'M SLIPPING...",
    "IT HAPPENED BEFORE. IT WILL HAPPEN AGAIN.",
};

const char *const TKW_SECRET_TEXT = "SOMEONE WON YOU AT A PIER STALL, ONCE.";

const char *const TKW_INTRO[] = {
    "BURL HAD NOT MEANT TO GO ANYWHERE.",
    "HE HAD FOUND THE WARMEST PATCH OF ICE ON THE WHOLE FLOE, AND HE MEANT TO SLEEP ON IT ALL AFTERNOON.",
    "BUT SOMEWHERE DOWN IN HIS NAP, A VOICE BEGAN TO CALL HIM...",
    NULL,
};

/* fewer than 50 shells: the thread is thin */
const char *const TKW_TALK_GOLD[] = {
    "BURL! AT LAST, YOU'VE COME.",
    "BUT WE HAVE SO LITTLE TIME. IN THIS DREAM, SHELLS ARE THE THREAD BETWEEN YOU AND ME...",
    "AND YOU BRING ONLY %d. THE THREAD IS THIN, SO HEAR ME QUICKLY.",
    "THE LONG-LEGS ARE COMING. THEY SAIL ON HOLLOW TREES, AND THEY ARE NEARLY AT YOUR SHORE.",
    "THEY MEAN TO TAKE YOUR TUSKS. EVERY ONE OF YOU.",
    "ALL I CAN TELL YOU IS THIS: OFF THE ICE! INTO THE WATER, ALL OF YOU, AND SWIM!",
    "WAKE UP, BURL. WAKE THE HERD!",
    NULL,
};

/* fifty or more: time enough to say it all */
const char *const TKW_TALK_CHERRY[] = {
    "YOU DREAM DEEPLY, YOUNG BURL. LET ME SHOW MY FACE.",
    "I AM SKERRY. I SWAM THESE SEAS BEFORE THE ICE HAD A NAME, AND YOU ARE MY CHILD'S CHILD'S CHILD, MORE OR LESS.",
    "WALRUSES WERE GREAT ONCE. MY FAMILY FORGOT IT ALL, AND FOR AGES I CALLED AND NONE OF THEM HEARD.",
    "BUT YOU HEARD. TOOK YOUR TIME ABOUT IT, MIND.",
    "NOW LISTEN. THE LONG-LEGS ARE COMING ON THEIR HOLLOW TREES. THEY WANT YOUR TUSKS.",
    "BUT THE SEA IS OUR GRANDMOTHER, AND SHE STILL OWES ME A KINDNESS.",
    "WHEN YOU WAKE, FACE THE WATER. WHEN THE HOLLOW TREE COMES ROUND THE POINT...",
    "...SLAP THE ICE THREE TIMES WITH YOUR FLIPPER. LIKE SO. ONE. TWO. THREE.",
    "SHE WILL DO THE REST. NOW WAKE UP, BURL.",
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
    if (!cherry && strstr(pages[i], "%d")) {
        snprintf(buf, (size_t)n, "AND YOU BRING ONLY %d. THE THREAD IS THIN, SO HEAR ME QUICKLY.", tkg.shells);
        return buf;
    }
    return pages[i];
}
