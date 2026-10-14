/* WITHERLEA - every hotspot in the house: id, room, kind, the rectangle on
 * the screen, where an exit leads and which way its cursor points, and
 * what A says with nothing in hand (NULL: wth_logic.c decides). Included
 * by wth.h for the ids and by wth_data.c for the table.
 *
 * Indoors every picture shares one box: the back wall from x 72 to 248 and
 * y 26 to 100, side-wall doors at x 20-54 (left) and 266-300 (right), back
 * doors 58 px tall standing on y 100, and "turn back" along the bottom. */
#ifndef WTH_SPOTS_H
#define WTH_SPOTS_H

#define WTH_BACK_EXIT(id, room, dest) X(id, room, HS_EXIT, 104, 124, 112, 9, dest, DIR_DOWN, NULL)
#define WTH_LEFT_EXIT(id, room, dest) X(id, room, HS_EXIT, 20, 40, 34, 72, dest, DIR_LEFT, NULL)
#define WTH_RIGHT_EXIT(id, room, dest) X(id, room, HS_EXIT, 266, 40, 34, 72, dest, DIR_RIGHT, NULL)

#define WTH_SPOT_LIST(X)                                                                                        \
    /* the cold store */                                                                                        \
    X(S_CELL_DOOR, R_CELL, HS_THING, 136, 42, 48, 58, 0, 0, NULL)                                               \
    X(S_CELL_EXIT, R_CELL, HS_EXIT, 136, 42, 48, 58, R_PASSAGE, DIR_UP, NULL)                                   \
    X(S_CELL_VENT, R_CELL, HS_THING, 24, 30, 30, 18, 0, 0, NULL)                                                \
    X(S_CELL_STRAW, R_CELL, HS_THING, 24, 102, 90, 20, 0, 0, NULL)                                              \
    X(S_CELL_TRAY, R_CELL, HS_THING, 206, 108, 44, 14, 0, 0, NULL)                                              \
    X(S_CELL_BARREL, R_CELL, HS_THING, 262, 64, 34, 52, 0, 0,                                                   \
      "A SALT BARREL. EMPTY, BUT IT STILL SMELLS OF EELS.")                                                      \
    X(S_CELL_RING, R_CELL, HS_THING, 96, 46, 16, 16, 0, 0,                                                      \
      "AN IRON RING SET IN THE WALL. SOMETHING WAS TIED TO IT ONCE, AND IT PULLED HARD.")                       \
    /* the back passage */                                                                                      \
    X(S_PAS_COAT, R_PASSAGE, HS_THING, 70, 42, 22, 42, 0, 0, NULL)                                              \
    X(S_PAS_RUG, R_PASSAGE, HS_THING, 104, 104, 112, 16, 0, 0, NULL)                                            \
    X(S_PAS_BAROMETER, R_PASSAGE, HS_THING, 224, 44, 14, 28, 0, 0,                                              \
      "A BAROMETER. THE NEEDLE HAS GONE PAST STORMY, TO A WORD SOMEONE SCRATCHED INTO THE DIAL: SOON.")         \
    X(S_PAS_HALL, R_PASSAGE, HS_EXIT, 142, 42, 36, 58, R_HALL, DIR_UP, NULL)                                     \
    WTH_LEFT_EXIT(S_PAS_SICK, R_PASSAGE, R_SICKROOM)                                                            \
    WTH_RIGHT_EXIT(S_PAS_KITCHEN, R_PASSAGE, R_KITCHEN)                                                         \
    WTH_BACK_EXIT(S_PAS_CELL, R_PASSAGE, R_CELL)                                                                \
    /* the sickroom */                                                                                          \
    X(S_SICK_BED, R_SICKROOM, HS_THING, 168, 70, 112, 46, 0, 0, NULL)                                           \
    X(S_SICK_STAND, R_SICKROOM, HS_THING, 128, 78, 30, 28, 0, 0, NULL)                                          \
    X(S_SICK_WASH, R_SICKROOM, HS_THING, 24, 62, 40, 46, 0, 0,                                                  \
      "A WASHSTAND. THE WATER IN THE JUG HAS GROWN A SKIN, PALE AS MILK.")                                       \
    X(S_SICK_WINDOW, R_SICKROOM, HS_THING, 96, 32, 46, 34, 0, 0,                                                \
      "THE WINDOW IS NAILED SHUT FROM THIS SIDE. OUT THERE THE FENS ARE BLACK AS INK.")                         \
    WTH_BACK_EXIT(S_SICK_OUT, R_SICKROOM, R_PASSAGE)                                                            \
    /* the entrance hall */                                                                                     \
    X(S_HALL_FRONT, R_HALL, HS_THING, 138, 38, 44, 62, 0, 0,                                                    \
      "THE FRONT DOOR. AN IRON BAR ACROSS IT, A CHAIN ROUND THE BAR, AND THE PADLOCK FILLED WITH LEAD.")       \
    X(S_HALL_CLOCK, R_HALL, HS_THING, 80, 30, 24, 70, 0, 0, NULL)                                               \
    X(S_HALL_UMBRELLA, R_HALL, HS_THING, 110, 80, 18, 22, 0, 0, NULL)                                           \
    X(S_HALL_STAIRS_DARK, R_HALL, HS_THING, 254, 26, 52, 92, 0, 0, NULL)                                        \
    X(S_HALL_STAIRS, R_HALL, HS_EXIT, 254, 26, 52, 92, R_LANDING, DIR_UP, NULL)                                 \
    X(S_HALL_DINING, R_HALL, HS_EXIT, 198, 42, 30, 58, R_DINING, DIR_UP, NULL)                                  \
    WTH_LEFT_EXIT(S_HALL_PARLOUR, R_HALL, R_PARLOUR)                                                            \
    WTH_BACK_EXIT(S_HALL_BACK, R_HALL, R_PASSAGE)                                                               \
    /* the parlour */                                                                                           \
    X(S_PAR_PIG, R_PARLOUR, HS_THING, 150, 38, 20, 12, 0, 0, NULL)                                              \
    X(S_PAR_FIRE, R_PARLOUR, HS_THING, 126, 62, 68, 38, 0, 0, NULL)                                             \
    X(S_PAR_RADIO, R_PARLOUR, HS_THING, 206, 62, 32, 38, 0, 0, NULL)                                            \
    X(S_PAR_SCREEN, R_PARLOUR, HS_HIDE, 78, 44, 40, 56, 0, 0, NULL)                                             \
    X(S_PAR_SOFA, R_PARLOUR, HS_THING, 60, 96, 110, 24, 0, 0, NULL)                                             \
    WTH_LEFT_EXIT(S_PAR_SMOKING, R_PARLOUR, R_SMOKING)                                                          \
    WTH_RIGHT_EXIT(S_PAR_GLASS, R_PARLOUR, R_GLASSHOUSE)                                                        \
    WTH_BACK_EXIT(S_PAR_BACK, R_PARLOUR, R_HALL)                                                                \
    /* the dining room */                                                                                       \
    X(S_DIN_ORCH, R_DINING, HS_THING, 82, 34, 36, 66, 0, 0, NULL)                                               \
    X(S_DIN_SIDE, R_DINING, HS_THING, 194, 66, 48, 34, 0, 0, NULL)                                              \
    X(S_DIN_PORTRAIT, R_DINING, HS_THING, 134, 32, 50, 32, 0, 0,                                                \
      "A FAMILY OF FOUR IN THEIR SUNDAY BEST. THE FATHER'S FACE HAS BEEN SCRAPED OFF WITH A KNIFE, NEATLY, "    \
      "LIKE A JOB SOMEONE TOOK THEIR TIME OVER.")                                                               \
    X(S_DIN_TABLE, R_DINING, HS_THING, 70, 94, 180, 24, 0, 0, NULL)                                             \
    WTH_RIGHT_EXIT(S_DIN_KITCHEN, R_DINING, R_KITCHEN)                                                          \
    WTH_BACK_EXIT(S_DIN_BACK, R_DINING, R_HALL)                                                                 \
    /* the kitchen */                                                                                           \
    X(S_KIT_STOVE, R_KITCHEN, HS_THING, 198, 54, 46, 46, 0, 0, NULL)                                            \
    X(S_KIT_KNIVES, R_KITCHEN, HS_THING, 264, 78, 22, 16, 0, 0, NULL)                                           \
    X(S_KIT_BOOK, R_KITCHEN, HS_THING, 134, 34, 52, 16, 0, 0, NULL)                                             \
    X(S_KIT_SINK, R_KITCHEN, HS_THING, 126, 70, 64, 30, 0, 0,                                                   \
      "A STONE SINK STACKED WITH JARS. EVERY JAR HOLDS THE SAME PALE HONEY, AND EVERY LID HAS A DATE ON IT.")   \
    X(S_KIT_SCULLERY, R_KITCHEN, HS_EXIT, 82, 42, 30, 58, R_SCULLERY, DIR_UP, NULL)                             \
    WTH_LEFT_EXIT(S_KIT_DINING, R_KITCHEN, R_DINING)                                                            \
    WTH_BACK_EXIT(S_KIT_BACK, R_KITCHEN, R_PASSAGE)                                                             \
    /* the scullery */                                                                                          \
    X(S_SCU_TUB, R_SCULLERY, HS_THING, 70, 84, 64, 34, 0, 0, NULL)                                              \
    X(S_SCU_SODA, R_SCULLERY, HS_THING, 150, 36, 16, 16, 0, 0, NULL)                                            \
    X(S_SCU_DIARY, R_SCULLERY, HS_THING, 176, 40, 14, 18, 0, 0, NULL)                                           \
    X(S_SCU_MANGLE, R_SCULLERY, HS_THING, 198, 60, 42, 40, 0, 0,                                                \
      "A MANGLE. THE ROLLERS ARE CRUSTED WITH SOMETHING WAXY AND WHITE THAT WON'T SCRAPE OFF.")                \
    X(S_SCU_GARDEN, R_SCULLERY, HS_EXIT, 90, 42, 30, 58, R_GARDEN, DIR_UP, NULL)                                \
    WTH_RIGHT_EXIT(S_SCU_GARAGE, R_SCULLERY, R_GARAGE)                                                          \
    WTH_BACK_EXIT(S_SCU_BACK, R_SCULLERY, R_KITCHEN)                                                            \
    /* the smoking room */                                                                                      \
    X(S_SMO_HUMIDOR, R_SMOKING, HS_THING, 196, 72, 24, 14, 0, 0, NULL)                                          \
    X(S_SMO_DESK, R_SMOKING, HS_THING, 120, 70, 70, 32, 0, 0, NULL)                                             \
    X(S_SMO_ALCOVE, R_SMOKING, HS_HIDE, 80, 34, 34, 66, 0, 0, NULL)                                             \
    X(S_SMO_CHESS, R_SMOKING, HS_THING, 90, 104, 44, 14, 0, 0,                                                  \
      "A GAME LEFT HALF PLAYED. BLACK HAS NOTHING LEFT BUT THE KING, AND THE KING HAS BEEN SNAPPED IN TWO.")   \
    X(S_SMO_CHAIR, R_SMOKING, HS_THING, 206, 90, 54, 30, 0, 0, NULL)                                            \
    WTH_LEFT_EXIT(S_SMO_WASH, R_SMOKING, R_WASHROOM)                                                            \
    WTH_RIGHT_EXIT(S_SMO_GUEST, R_SMOKING, R_GUEST)                                                             \
    WTH_BACK_EXIT(S_SMO_BACK, R_SMOKING, R_PARLOUR)                                                             \
    /* the washroom */                                                                                          \
    X(S_WAS_BATH, R_WASHROOM, HS_THING, 128, 72, 112, 36, 0, 0, NULL)                                           \
    X(S_WAS_CAB, R_WASHROOM, HS_THING, 84, 34, 32, 30, 0, 0, NULL)                                              \
    X(S_WAS_TOILET, R_WASHROOM, HS_THING, 36, 72, 30, 42, 0, 0, NULL)                                           \
    WTH_BACK_EXIT(S_WAS_BACK, R_WASHROOM, R_SMOKING)                                                            \
    /* the guest room */                                                                                        \
    X(S_GUE_BOX, R_GUEST, HS_THING, 196, 72, 40, 18, 0, 0, NULL)                                                \
    X(S_GUE_SEW, R_GUEST, HS_THING, 24, 70, 44, 40, 0, 0, NULL)                                                 \
    X(S_GUE_BED, R_GUEST, HS_HIDE, 92, 72, 96, 40, 0, 0, NULL)                                                  \
    X(S_GUE_WINDOW, R_GUEST, HS_THING, 144, 32, 40, 30, 0, 0,                                                   \
      "BEYOND THE GLASS, THE BEEHIVES IN THE GARDEN. NOTHING FLIES.")                                          \
    WTH_BACK_EXIT(S_GUE_BACK, R_GUEST, R_SMOKING)                                                               \
    /* the glasshouse */                                                                                        \
    X(S_GLA_TRELLIS, R_GLASSHOUSE, HS_THING, 30, 30, 50, 70, 0, 0, NULL)                                        \
    X(S_GLA_PLANTS, R_GLASSHOUSE, HS_THING, 196, 84, 84, 34, 0, 0,                                              \
      "TOMATO PLANTS DEAD ON THEIR CANES. A FINE WHITE FUR COVERS EVERY STEM.")                                \
    X(S_GLA_PANE, R_GLASSHOUSE, HS_THING, 226, 16, 44, 40, 0, 0, NULL)                                          \
    X(S_GLA_GARDEN, R_GLASSHOUSE, HS_EXIT, 128, 34, 64, 66, R_GARDEN, DIR_UP, NULL)                             \
    WTH_BACK_EXIT(S_GLA_BACK, R_GLASSHOUSE, R_PARLOUR)                                                          \
    /* the garage */                                                                                            \
    X(S_GAR_OPENER, R_GARAGE, HS_THING, 138, 8, 44, 14, 0, 0, NULL)                                             \
    X(S_GAR_BUTTON, R_GARAGE, HS_THING, 26, 58, 14, 16, 0, 0, NULL)                                             \
    X(S_GAR_SPANNER, R_GARAGE, HS_THING, 254, 40, 20, 12, 0, 0, NULL)                                           \
    X(S_GAR_TWINE, R_GARAGE, HS_THING, 278, 44, 18, 14, 0, 0, NULL)                                             \
    X(S_GAR_SHELF, R_GARAGE, HS_THING, 260, 76, 34, 20, 0, 0, NULL)                                             \
    X(S_GAR_CAR, R_GARAGE, HS_THING, 96, 62, 128, 54, 0, 0, NULL)                                               \
    X(S_GAR_DOOR, R_GARAGE, HS_THING, 76, 28, 168, 32, 0, 0, NULL)                                              \
    X(S_GAR_OUT, R_GARAGE, HS_EXIT, 76, 28, 168, 32, R_DRIVE, DIR_UP, NULL)                                     \
    WTH_BACK_EXIT(S_GAR_BACK, R_GARAGE, R_SCULLERY)                                                             \
    /* the pond */                                                                                              \
    X(S_POND_BODY, R_POND, HS_THING, 128, 76, 64, 18, 0, 0, NULL)                                               \
    X(S_POND_LILIES, R_POND, HS_THING, 30, 92, 70, 20, 0, 0,                                                    \
      "WATER LILIES, EVERY ONE OF THEM WHITE. THEY'RE NOT FLOWERS. THEY'RE TOO SOFT.")                         \
    X(S_POND_REEDS, R_POND, HS_THING, 236, 60, 60, 50, 0, 0,                                                    \
      "TALL REEDS, BENT FLAT IN ONE PLACE, AS IF SOMETHING HEAVY WAS DRAGGED THROUGH.")                        \
    WTH_BACK_EXIT(S_POND_BACK, R_POND, R_GARDEN)                                                                \
    /* the potting shed */                                                                                      \
    X(S_SHE_GRINDER, R_SHED, HS_THING, 30, 70, 44, 26, 0, 0, NULL)                                              \
    X(S_SHE_VISE, R_SHED, HS_THING, 98, 72, 26, 22, 0, 0, NULL)                                                 \
    X(S_SHE_DIARY, R_SHED, HS_THING, 78, 74, 16, 12, 0, 0, NULL)                                                \
    X(S_SHE_SCREW, R_SHED, HS_THING, 146, 34, 12, 24, 0, 0, NULL)                                               \
    X(S_SHE_SHEARS, R_SHED, HS_THING, 164, 34, 20, 26, 0, 0, NULL)                                              \
    X(S_SHE_SAW, R_SHED, HS_THING, 190, 36, 34, 16, 0, 0, NULL)                                                 \
    X(S_SHE_SPADE, R_SHED, HS_THING, 250, 36, 18, 74, 0, 0, NULL)                                               \
    X(S_SHE_SACK, R_SHED, HS_THING, 196, 92, 40, 28, 0, 0, NULL)                                                \
    WTH_BACK_EXIT(S_SHE_BACK, R_SHED, R_GARDEN)                                                                 \
    /* the side garden */                                                                                       \
    X(S_GDN_HANG, R_GARDEN, HS_THING, 36, 8, 18, 40, 0, 0,                                                      \
      "HIGH ON THE BACK OF THE HOUSE, SOMEONE HANGS FROM THE BALCONY RAIL BY A ROPE. FAR TOO HIGH TO REACH.")   \
    X(S_GDN_FALLEN, R_GARDEN, HS_THING, 26, 108, 54, 14, 0, 0, NULL)                                            \
    X(S_GDN_YEW, R_GARDEN, HS_THING, 140, 22, 62, 66, 0, 0,                                                     \
      "AN OLD YEW, BLACK AND BROAD. THE GROUND UNDER IT HAS BEEN DUG AND PATTED DOWN.")                         \
    X(S_GDN_GRAVE, R_GARDEN, HS_THING, 150, 96, 40, 16, 0, 0, NULL)                                             \
    X(S_GDN_LAUREL, R_GARDEN, HS_HIDE, 100, 92, 44, 30, 0, 0, NULL)                                             \
    X(S_GDN_HIVES, R_GARDEN, HS_THING, 196, 104, 42, 18, 0, 0,                                                  \
      "THREE BEEHIVES. NOT ONE HUMS. EVERY ENTRANCE IS SEALED WITH WHITE WAX, FROM THE INSIDE.")               \
    X(S_GDN_IVY, R_GARDEN, HS_THING, 236, 38, 30, 58, 0, 0, NULL)                                               \
    X(S_GDN_ARCH, R_GARDEN, HS_EXIT, 236, 38, 30, 58, R_COURTYARD, DIR_UP, NULL)                                \
    X(S_GDN_SHEDDOOR, R_GARDEN, HS_THING, 272, 46, 34, 54, 0, 0, NULL)                                          \
    X(S_GDN_SHED, R_GARDEN, HS_EXIT, 272, 46, 34, 54, R_SHED, DIR_RIGHT, NULL)                                  \
    X(S_GDN_SCULLERY, R_GARDEN, HS_EXIT, 20, 54, 26, 48, R_SCULLERY, DIR_LEFT, NULL)                            \
    X(S_GDN_GLASS, R_GARDEN, HS_EXIT, 56, 50, 36, 42, R_GLASSHOUSE, DIR_LEFT, NULL)                             \
    X(S_GDN_POND, R_GARDEN, HS_EXIT, 206, 76, 26, 20, R_POND, DIR_UP, NULL)                                     \
    WTH_BACK_EXIT(S_GDN_DRIVE, R_GARDEN, R_DRIVE)                                                               \
    /* the front drive */                                                                                       \
    X(S_DRV_FRONT, R_DRIVE, HS_THING, 34, 46, 30, 54, 0, 0,                                                     \
      "THE FRONT DOOR, FROM OUTSIDE. THE WINDOWS BESIDE IT ARE BOARDED. SOMEONE MEANT THE HOUSE TO STAY SHUT.")  \
    X(S_DRV_GARAGE, R_DRIVE, HS_THING, 226, 50, 74, 52, 0, 0, NULL)                                             \
    X(S_DRV_GARAGE_IN, R_DRIVE, HS_EXIT, 226, 50, 74, 52, R_GARAGE, DIR_UP, NULL)                               \
    X(S_DRV_STATUE, R_DRIVE, HS_THING, 146, 36, 28, 44, 0, 0, NULL)                                             \
    X(S_DRV_FOUNTAIN, R_DRIVE, HS_THING, 116, 82, 88, 26, 0, 0, NULL)                                           \
    X(S_DRV_STORM, R_DRIVE, HS_THING, 76, 106, 42, 16, 0, 0, NULL)                                              \
    X(S_DRV_CELLAR, R_DRIVE, HS_EXIT, 76, 106, 42, 16, R_CELLAR, DIR_DOWN, NULL)                                \
    X(S_DRV_GATES, R_DRIVE, HS_THING, 238, 108, 70, 16, 0, 0,                                                   \
      "THE DRIVE RUNS DOWN TO THE GATES. THEY'RE CHAINED, BUT A CAR WOULD GO THROUGH THEM.")                   \
    X(S_DRV_GARDEN, R_DRIVE, HS_EXIT, 10, 104, 40, 22, R_GARDEN, DIR_LEFT, NULL)                                \
    /* the courtyard */                                                                                         \
    X(S_CRT_HARE0, R_COURTYARD, HS_THING, 46, 40, 34, 40, 0, 0, NULL)                                           \
    X(S_CRT_HARE1, R_COURTYARD, HS_THING, 92, 40, 34, 40, 0, 0, NULL)                                           \
    X(S_CRT_HARE2, R_COURTYARD, HS_THING, 138, 40, 34, 40, 0, 0, NULL)                                          \
    X(S_CRT_HARE3, R_COURTYARD, HS_THING, 184, 40, 34, 40, 0, 0, NULL)                                          \
    X(S_CRT_PLAQUE, R_COURTYARD, HS_THING, 96, 88, 76, 14, 0, 0,                                                \
      "A PLAQUE: FIRST THE SUN COMES UP. THEN THE LEAF OPENS. THE LEAF FEEDS ON BLOOD. THE BLOOD BELONGS "     \
      "TO THE MOON.")                                                                                           \
    X(S_CRT_GATE, R_COURTYARD, HS_THING, 246, 32, 50, 76, 0, 0, NULL)                                           \
    X(S_CRT_MAZE, R_COURTYARD, HS_EXIT, 246, 32, 50, 76, R_MAZE, DIR_RIGHT, NULL)                               \
    WTH_BACK_EXIT(S_CRT_BACK, R_COURTYARD, R_GARDEN)                                                            \
    /* the hedge maze */                                                                                        \
    X(S_MAZ_LEFT, R_MAZE, HS_EXIT, 18, 34, 54, 80, R_MAZE, DIR_LEFT, NULL)                                      \
    X(S_MAZ_AHEAD, R_MAZE, HS_EXIT, 134, 40, 52, 60, R_MAZE, DIR_UP, NULL)                                      \
    X(S_MAZ_RIGHT, R_MAZE, HS_EXIT, 248, 34, 54, 80, R_MAZE, DIR_RIGHT, NULL)                                   \
    WTH_BACK_EXIT(S_MAZ_BACK, R_MAZE, R_COURTYARD)                                                              \
    X(S_HRT_GIRL, R_HEART, HS_THING, 134, 26, 52, 84, 0, 0, NULL)                                               \
    X(S_HRT_BENCH, R_HEART, HS_THING, 30, 92, 70, 22, 0, 0,                                                     \
      "A STONE BENCH. TWO SMALL HANDPRINTS ARE PRESSED INTO THE MOSS ON IT, AS IF SOMEONE SAT HERE A LONG TIME.") \
    WTH_BACK_EXIT(S_HRT_BACK, R_HEART, R_COURTYARD)                                                             \
    /* the landing */                                                                                           \
    X(S_LAN_HATCH, R_LANDING, HS_THING, 132, 8, 56, 14, 0, 0, NULL)                                             \
    X(S_LAN_ATTIC, R_LANDING, HS_EXIT, 132, 8, 56, 14, R_ATTIC, DIR_UP, NULL)                                   \
    X(S_LAN_MASTER_SHUT, R_LANDING, HS_THING, 20, 40, 34, 72, 0, 0, NULL)                                       \
    WTH_LEFT_EXIT(S_LAN_MASTER, R_LANDING, R_MASTER)                                                            \
    X(S_LAN_STUDY, R_LANDING, HS_EXIT, 80, 42, 30, 58, R_STUDY, DIR_UP, NULL)                                   \
    X(S_LAN_CHILDREN, R_LANDING, HS_EXIT, 145, 42, 30, 58, R_CHILDREN, DIR_UP, NULL)                            \
    X(S_LAN_BATH, R_LANDING, HS_EXIT, 210, 42, 30, 58, R_BATH, DIR_UP, NULL)                                    \
    WTH_RIGHT_EXIT(S_LAN_NURSERY, R_LANDING, R_NURSERY)                                                         \
    WTH_BACK_EXIT(S_LAN_STAIRS, R_LANDING, R_HALL)                                                              \
    /* the master bedroom */                                                                                    \
    X(S_MAS_BED, R_MASTER, HS_THING, 112, 66, 104, 46, 0, 0, NULL)                                              \
    X(S_MAS_DRESSER, R_MASTER, HS_THING, 26, 58, 42, 48, 0, 0, NULL)                                            \
    X(S_MAS_SAFE, R_MASTER, HS_THING, 222, 40, 26, 26, 0, 0, NULL)                                              \
    WTH_RIGHT_EXIT(S_MAS_BALC, R_MASTER, R_BALCONY)                                                             \
    WTH_BACK_EXIT(S_MAS_BACK, R_MASTER, R_LANDING)                                                              \
    /* the balcony */                                                                                           \
    X(S_BAL_ROPE, R_BALCONY, HS_THING, 148, 52, 24, 58, 0, 0, NULL)                                             \
    X(S_BAL_VIEW, R_BALCONY, HS_THING, 24, 14, 100, 40, 0, 0,                                                   \
      "THE FENS GO ON FOREVER UNDER NO MOON. FAR AWAY ONE LIGHT BURNS, THEN DOESN'T.")                         \
    WTH_BACK_EXIT(S_BAL_BACK, R_BALCONY, R_MASTER)                                                              \
    /* the study */                                                                                             \
    X(S_STU_COMPUTER, R_STUDY, HS_THING, 128, 52, 54, 42, 0, 0, NULL)                                           \
    X(S_STU_CASE, R_STUDY, HS_THING, 24, 56, 46, 36, 0, 0, NULL)                                                \
    X(S_STU_DIARY, R_STUDY, HS_THING, 188, 84, 18, 12, 0, 0, NULL)                                              \
    X(S_STU_SHELF, R_STUDY, HS_THING, 214, 32, 54, 66, 0, 0,                                                    \
      "BOOKS ON BEES, ON MOULDS, ON THE CHEMISTRY OF SLEEP. A LOT OF PAGES HAVE BEEN TORN OUT AND EATEN.")      \
    WTH_BACK_EXIT(S_STU_BACK, R_STUDY, R_LANDING)                                                               \
    /* the bathroom upstairs */                                                                                 \
    X(S_BAT_TUB, R_BATH, HS_THING, 128, 72, 112, 36, 0, 0, NULL)                                                \
    X(S_BAT_MIRROR, R_BATH, HS_THING, 84, 30, 30, 32, 0, 0, NULL)                                               \
    X(S_BAT_SHELF, R_BATH, HS_THING, 252, 40, 26, 24, 0, 0, NULL)                                               \
    X(S_BAT_TOILET, R_BATH, HS_THING, 32, 72, 28, 42, 0, 0, NULL)                                               \
    X(S_BAT_DIARY, R_BATH, HS_THING, 92, 108, 16, 12, 0, 0, NULL)                                               \
    WTH_BACK_EXIT(S_BAT_BACK, R_BATH, R_LANDING)                                                                \
    /* the nursery */                                                                                           \
    X(S_NUR_BOX, R_NURSERY, HS_THING, 118, 74, 26, 16, 0, 0, NULL)                                              \
    X(S_NUR_WALL, R_NURSERY, HS_THING, 166, 32, 74, 40, 0, 0, NULL)                                             \
    X(S_NUR_CHEST, R_NURSERY, HS_HIDE, 30, 84, 62, 32, 0, 0, NULL)                                              \
    X(S_NUR_HORSE, R_NURSERY, HS_THING, 236, 70, 52, 44, 0, 0,                                                  \
      "A ROCKING HORSE. IT ROCKS, VERY GENTLY, WHEN NOTHING IS TOUCHING IT.")                                   \
    X(S_NUR_DIARY, R_NURSERY, HS_THING, 104, 108, 16, 12, 0, 0, NULL)                                           \
    WTH_BACK_EXIT(S_NUR_BACK, R_NURSERY, R_LANDING)                                                             \
    /* the children's bedroom */                                                                                \
    X(S_CHI_HOUSE, R_CHILDREN, HS_THING, 30, 56, 72, 50, 0, 0, NULL)                                            \
    X(S_CHI_DOLL, R_CHILDREN, HS_THING, 140, 88, 22, 22, 0, 0, NULL)                                            \
    X(S_CHI_DIARY, R_CHILDREN, HS_THING, 118, 38, 14, 16, 0, 0, NULL)                                           \
    X(S_CHI_BUNK, R_CHILDREN, HS_THING, 186, 34, 96, 76, 0, 0,                                                  \
      "BUNK BEDS, BOTH MADE UP TIGHT. ON THE TOP PILLOW, A DENT WHERE A HEAD WAS, STILL WARM.")                 \
    WTH_BACK_EXIT(S_CHI_BACK, R_CHILDREN, R_LANDING)                                                            \
    /* the attic */                                                                                             \
    X(S_ATT_TRUNK, R_ATTIC, HS_THING, 30, 88, 64, 30, 0, 0, NULL)                                               \
    X(S_ATT_HARM, R_ATTIC, HS_THING, 196, 58, 66, 46, 0, 0, NULL)                                               \
    X(S_ATT_DUMMIES, R_ATTIC, HS_HIDE, 112, 36, 64, 74, 0, 0, NULL)                                             \
    X(S_ATT_DIARY, R_ATTIC, HS_THING, 270, 106, 16, 12, 0, 0, NULL)                                             \
    WTH_BACK_EXIT(S_ATT_BACK, R_ATTIC, R_LANDING)                                                               \
    /* the cellar */                                                                                            \
    X(S_CEL_MECH, R_CELLAR, HS_THING, 92, 48, 30, 38, 0, 0, NULL)                                               \
    X(S_CEL_DOOR, R_CELLAR, HS_THING, 136, 40, 48, 60, 0, 0, NULL)                                              \
    X(S_CEL_SPORE, R_CELLAR, HS_EXIT, 136, 40, 48, 60, R_SPORE, DIR_UP, NULL)                                   \
    X(S_CEL_SHELF, R_CELLAR, HS_THING, 214, 42, 66, 62, 0, 0,                                                   \
      "SHELVES OF HONEY JARS, HUNDREDS OF THEM. THE OLDEST HAVE GONE SOLID AND WHITE, AND SOMETHING IN THEM "  \
      "HAS GROWN UP AGAINST THE GLASS.")                                                                        \
    X(S_CEL_DIARY, R_CELLAR, HS_THING, 36, 104, 16, 12, 0, 0, NULL)                                             \
    WTH_BACK_EXIT(S_CEL_BACK, R_CELLAR, R_DRIVE)                                                                \
    /* the white room */                                                                                        \
    X(S_SPO_FACE, R_SPORE, HS_THING, 108, 26, 104, 74, 0, 0, NULL)                                              \
    X(S_SPO_ROOTS, R_SPORE, HS_THING, 18, 70, 70, 50, 0, 0,                                                     \
      "WHITE THREADS AS THICK AS ROPE, ALL RUNNING THE SAME WAY: UP, INTO THE HOUSE.")                         \
    WTH_BACK_EXIT(S_SPO_BACK, R_SPORE, R_CELLAR)

#define X(id, ...) id,
enum { WTH_SPOT_LIST(X) S_COUNT };
#undef X

#endif
