#include "drivers.h"
#include <cstddef>

// References:
// https://en.wikipedia.org/wiki/List_of_most_popular_given_names
// https://en.wikipedia.org/wiki/Lists_of_most_common_surnames_in_Asian_countries
// https://familynames.org/

static const char *JP_surnames[] =
{
    "Sato",
    "Suzuki",
    "Takahashi",
    "Tanaka",
    "Watanabe",
    "Ito",
    "Nakamura",
    "Kobayashi",
    "Yamamoto",
    "Kato",
    "Yoshida",
    "Yamada",
    "Sasaki",
    "Yamaguchi",
    "Matsumoto",
    "Inoue",
    "Kimura",
    "Shimizu",
    "Hayashi",
    "Saito",
    "Yamazaki",
    "Nakajima",
    "Mori",
    "Abe",
};

static const char *JP_names[] =
{
    "Aoi",
    "So",
    "Ao",
    "Nagi",
    "Nagisa",
    "Ren",
    "Haruto",
    "Hinato",
    "Minato",
    "Soma",
    "Fuma",
    "Ao",
    "Aoi",
    "Itsuki",
    "Tatsuki",
    "Yamato",
    "Yuma",
    "Haruma",
    "Dan",
    "Haru",
};

static const char *DE_surnames[] =
{
    "Müller",
    "Schmidt",
    "Schneider",
    "Fischer",
    "Weber",
    "Meyer",
    "Wagner",
    "Becker",
    "Schulz",
    "Hoffmann",
    "Schafer",
    "Koch",
    "Bauer",
    "Richter",
    "Klein",
    "Wolf",
    "Schroeder",
    "Neumann",
    "Schwarz",
    "Zimmermann",
    "Braun",
    "Krueger",
    "Hofmann",
    "Hartmann",
    "Lange",
};

static const char *DE_names[] =
{
    "Noah",
    "Matteo",
    "Elias",
    "Finn",
    "Leon",
    "Theo",
    "Paul",
    "Emil",
    "Henry",
    "Ben",
};

static const char *GB_names[] =
{
    "Noah",
    "Muhammad",
    "George",
    "Oliver",
    "Leo",
    "Arthur",
    "Oscar",
    "Theodore",
    "Freddie",
    "Theo",
};

static const char *GB_surnames[] =
{
    "Smith",
    "Johnson",
    "Williams",
    "Brown",
    "Jones",
    "Miller",
    "Davis",
    "Garcia",
    "Rodriguez",
    "Wilson",
    "Martinez",
    "Anderson",
    "Taylor",
    "Thomas",
    "Hernandez",
    "Moore",
    "Martin",
    "Jackson",
    "Thompson",
    "White",
    "Lopez",
    "Lee",
    "Gonzalez",
    "Harris",
    "Clark",
};

static const char *const CR_names[] =
{
    "Mia",
    "Nika",
    "Rita",
    "Mila",
    "Ema",
    "Lucija",
    "Marta",
    "Sara",
    "Eva",
    "Elena",
    "Luka",
    "Jakov",
    "David",
    "Mateo",
    "Toma",
    "Roko",
    "Petar",
    "Matej",
    "Fran",
    "Ivan",
};

static const char *const CR_surnames[] =
{
    "Horvat",
    "Kovačević",
    "Babić",
    "Marić",
    "Jurić",
    "Novak",
    "Kovačić",
    "Knežević",
    "Vuković",
    "Marković",
    "Petrović",
    "Matić",
    "Tomić",
    "Pavlović",
    "Kovač",
    "Božić",
    "Blažević",
    "Grgić",
    "Pavić",
    "Radić",
    "Perić",
    "Filipović",
    "Šarić",
    "Lovrić",
    "Vidović",
    "Perković",
    "Popović",
    "Bošnjak",
    "Jukić",
    "Barišić",
};

static const char *const IT_names[] =
{
    "Sofia",
    "Aurora",
    "Giulia",
    "Ginevra",
    "Vittoria",
    "Beatrice",
    "Alice",
    "Ludovica",
    "Emma",
    "Matilde",
    "Leonardo",
    "Francesco",
    "Tommaso",
    "Edoardo",
    "Alessandro",
    "Lorenzo",
    "Mattia",
    "Gabriele",
    "Riccardo",
    "Andrea",
};

static const char *const IT_surnames[] =
{
    "Rossi",
    "Russo",
    "Ferrari",
    "Esposito",
    "Bianchi",
    "Romano",
    "Colombo",
    "Bruno",
    "Ricci",
    "Greco",
    "Marino",
    "Gallo",
    "De Luca",
    "Conti",
    "Costa",
    "Mancini",
    "Giordano",
    "Rizzo",
    "Lombardi",
    "Barbieri",
    "Moretti",
    "Fontana",
    "Caruso",
    "Mariani",
    "Ferrara",
};

static const char *const FR_names[] =
{
    "Louise",
    "Ambre",
    "Alba",
    "Jade",
    "Emma",
    "Rose",
    "Alma",
    "Alice",
    "Romy",
    "Anna",
    "Gabriel",
    "Raphael",
    "Leo",
    "Louis",
    "Mael",
    "Noah",
    "Jules",
    "Adam",
    "Arthur",
    "Isaac",
};

static const char *const FR_surnames[] =
{
    "Martin",
    "Bernard",
    "Dubois",
    "Thomas",
    "Robert",
    "Richard",
    "Petit",
    "Durand",
    "Leroy",
    "Moreau",
    "Simon",
    "Laurent",
    "Lefebvre",
    "Michel",
    "Garcia",
    "David",
    "Bertrand",
    "Roux",
    "Vincent",
    "Fournier",
    "Morel",
    "Girard",
    "Andre",
    "Lefevre",
    "Mercier",
};

static const char *const IN_names[] =
{
    "Sunita",
    "Anita",
    "Gita",
    "Rekha",
    "Shanti",
    "Usha",
    "Asha",
    "Mina",
    "Laxmi",
    "Sita",
    "Shivansh",
    "Dhruv",
    "Kabir",
    "Vedant",
    "Kiaan",
    "Aarav",
    "Arjun",
    "Viraj",
    "Krishna",
    "Avyan",
};

static const char *const IN_surnames[] =
{
    "Devi",
    "Singh",
    "Kumar",
    "Das",
    "Kaur",
    "Ram",
    "Yadav",
    "Kumari",
    "Lal",
    "Bai",
};

static const char *const GR_names[] =
{
    "Giorgos",
    "Konstantinos",
    "Ioannis",
    "Dimitris",
    "Nikolaos",
    "Panagiotis",
    "Christos",
    "Alexandros",
    "Vasilis",
    "Angelos",
    "Aria",
    "Christina",
    "Ellie",
    "Maria",
    "Violeta",
    "Raphaela",
};

static const char *const GR_surnames[] =
{
    "Samaras",
    "Papoutsis",
    "Kritikos",
    "Aivaliotis",
    "Kontos",
    "Mytaras",
    "Koufos",
    "Mitsotakis",
    "Theodorakis",
    "Stamatelopoulos",
    "Papadopoulos",
    "Alexoudis",
    "Tsolakoglou",
};

static const char *const IL_names[] =
{
    "Ariel",
    "Lavi",
    "David",
    "Raphael",
    "Uri",
    "Michael",
    "Adam",
    "Charbel",
    "Jude",
    "Avigail",
    "Maryam",
    "Libi",
    "Tamar",
    "Lia",
    "Sarah",
    "Yael",
    "Ella",
};

static const char *const IL_surnames[] =
{
    "Cohen",
    "Levi",
    "Mizrachi",
    "Peretz",
    "Biton",
    "Dahan",
    "Avraham",
    "Friedman",
    "Malka",
    "Azoulay",
    "Katz",
    "Yosef",
    "Amar",
    "Ohayon",
};

static const char *const CN_names[] =
{
    "Yi nuò",
    "Zi hán",
    "Yu tóng",
    "Xin yí",
    "Ke xin",
    "Mèng yáo",
    "Yìchén",
    "Yuxuān",
    "Hàoyu",
    "Yuchén",
    "Zimò",
    "Yuháng",
    "Hàorán",
    "Ziháo"
};

static const char *const CN_surnames[] =
{
    "Zhou",
    "Suen",
    "Ng",
    "Zhu",
    "Lin",
    "Wang",
    "Li",
    "Zhang",
    "Chen",
    "Sun",
    "Gao",
    "Luo"
};

static const char *const EG_names[] =
{
    "Shaimaa",
    "Shahd",
    "Ashraqat",
    "Sahar",
    "Fatin",
    "Dalal",
    "Fajr",
    "Suha",
    "Rowan",
    "Hasnaa",
    "Hosna",
    "Mohamed",
    "Ahmed",
    "Mahmoud",
    "Mustafa",
    "Taha",
    "Khaled",
    "Hamza",
    "Bilal",
};

static const char *const EG_surnames[] =
{
    "Abdallah",
    "Amin",
    "Ali",
    "Kamel",
    "Khalil",
    "Zakaria",
    "Hamed",
    "Magdy",
    "Said",
    "Al-Ameen",
    "Sallom",
    "Ally",
};

static const char *const ZA_names[] =
{
    "Onalerona",
    "Melokuhle",
    "Lisakhanya",
    "Zanokuhle",
    "Lethabo",
    "Lesedi",
    "Omphile",
    "Amahle",
    "Naledi",
    "Nkanyezi",
    "Lethabo",
    "Nkazimulo",
    "Lubanzi",
    "Nkanyezi",
    "Langelihle",
    "Lethokuhle",
    "Junior",
    "Melokuhle",
    "Siphosethu",
    "Omphilet",
};

static const char *const ZA_surnames[] =
{
    "Dlamini",
    "Nkosi",
    "Ndlovu",
    "Khumalo",
    "Sithole",
    "Botha",
    "Mahlangu",
    "Mokoena",
    "Smith",
    "Naidoo",
    "Mkhize",
    "Mthembu",
    "Ngcobo",
    "Gumede",
};

static const char *const FI_names[] =
{
    "Olivia",
    "Oliver",
    "Eino",
    "Väinö",
    "Ellen",
    "Aino",
    "Linnea",
    "Sofia",
    "Leo",
    "Lilja",
    "Eevi",
    "Hilla",
    "Aava",
    "Helmi",
    "Isla",
    "Viola",
    "Elias",
    "Onni",
    "Toivo",
    "Oiva",
    "Emil",
    "Vilho",
    "Aatos",
    "Hugo",
};

static const char *const FI_surnames[] =
{
    "Korhonen",
    "Virtanen",
    "Mäkinen",
    "Nieminen",
    "Mäkelä",
    "Hämäläinen",
    "Laine",
    "Heikkinen",
    "Koskinen",
    "Järvinen",
    "Lehtonen",
    "Lehtinen",
    "Heinonen",
    "Salo",
};

#define NATION(X) \
{ \
    #X, \
    X##_names, \
    X##_surnames, \
    sizeof X##_names / sizeof *X##_names, \
    sizeof X##_surnames / sizeof *X##_surnames \
}

const struct GfDrivers::names GfDrivers::names[] =
{
    NATION(JP),
    NATION(GB),
    NATION(DE),
    NATION(CR),
    NATION(IT),
    NATION(FR),
    NATION(IN),
    NATION(GR),
    NATION(IL),
    NATION(CN),
    NATION(EG),
    NATION(ZA),
    NATION(FI),
};

const char *const GfDrivers::teams[] =
{
    "Rocketeer Racing",
    "Oli's Oil Team",
    "Velocity Racing",
    "Lovely BEAM",
    "Chaser Racing",
    "Nitro Racing",
    "Haruki Racing",
    "Iadanza Racing",
    "Asai Racing",
    "Japanese Toro Drivers Club",
    "Anime Age Racing",
    "Free French Racers team Orcana",
    "Speedstar Racing",
    "Takasuka Racing",
};

const size_t GfDrivers::n_names = sizeof names / sizeof *names;
const size_t GfDrivers::n_teams = sizeof teams / sizeof *teams;
