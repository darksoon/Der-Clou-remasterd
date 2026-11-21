/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
#include "scenes/scenes.h"

char *tcShowPriceOfTool(uint32_t nr, uint32_t type, void *data)
{
    static char line[TXT_KEY_LENGTH];
    char line1[TXT_KEY_LENGTH];
    Tool tool = (Tool)data;

    txtGetFirstLine(BUSINESS_TXT, "PRICE_AND_MONEY", line1);
    sprintf(line, line1, tool->Value);

    return line;
}

ubyte tcBuyTool(ubyte choice)
{
    LIST *tools = NULL;
    NODE *node = NULL;
    uint32_t toolID = 0;
    uint32_t price = 0;
    uint32_t count = 0;
    Person mary = (Person)dbGetObject(Person_Mary_Bolton);
    Tool tool = NULL;
    ubyte oldChoice = 0;
    char exp[TXT_KEY_LENGTH];

    ObjectListSuccString = tcShowPriceOfTool;
    ObjectListWidth = 48;

    hasAll(Person_Mary_Bolton,
           OLF_ALIGNED | OLF_PRIVATE_LIST | OLF_INCLUDE_NAME | OLF_INSERT_STAR | OLF_ADD_SUCC_STRING, Object_Tool);
    tools = ObjectListPrivate;

    ObjectListSuccString = NULL;
    ObjectListWidth = 0;

    txtGetFirstLine(BUSINESS_TXT, "THANKS", exp);
    ExpandObjectList(tools, exp);

    SetBubbleType(THINK_BUBBLE);

    count = GetNrOfNodes(tools) - 1;

    choice = min(choice, count);

    while (choice != GET_OUT)
    {
        SetPictID(MATT_PICTID);

        oldChoice = choice;

        if (ChoiceOk(choice = Bubble(tools, choice, 0L, 0L), GET_OUT, tools))
        {
            node = (NODE *)GetNthNode(tools, (uint32_t)choice);
            toolID = OL_NR(node);
            tool = (Tool)dbGetObject(toolID);
            price = tcGetToolPrice(tool);

            if (has(Person_Matt_Stuvysunt, toolID))
                Say(BUSINESS_TXT, 0, mary->PictID, "AUSVERKAUFT");
            else
            {
                if (tcSpendMoney(price, 0))
                {
                    hasSet(Person_Matt_Stuvysunt, toolID);
                    Say(BUSINESS_TXT, 0, mary->PictID, "GOOD TOOL");
                }
            }
        }
        else
            choice = GET_OUT;
    }

    RemoveList(tools);

    return oldChoice;
}

/* Game+: after the description of a tool, the skills it needs and who in the gang can use it */
static const char *toolNeeds[4] = {"Needs %s: %u of 100", "Braucht %s: %u von 100", "Demande %s : %u sur 100",
                                   "Necesita %s: %u de 100"};
static const char *toolCan[4] = {"Can use it: %s (%u)", "Kann damit umgehen: %s (%u)", "Peut l'utiliser : %s (%u)",
                                 "Puede usarla: %s (%u)"};
static const char *toolNobody[4] = {"Nobody in your gang can use it yet.", "Niemand in deiner Bande kann das bisher.",
                                    "Personne dans ta bande ne sait encore s'en servir.",
                                    "Nadie de tu banda sabe usarla todav\xa1" "a."};

static void tcAddToolUsers(LIST *desc, uint32_t toolId)
{
    uint32_t abilityId[8];
    uint32_t need[8];
    int count = 0;
    int users = 0;
    int i = 0;
    ubyte lang = txtGetLanguage();
    char line[TXT_KEY_LENGTH];
    char name[TXT_KEY_LENGTH];
    NODE *n = NULL;
    LIST *gang = NULL;

    if (lang > 3) lang = 1;

    toolRequiresAll(toolId, OLF_PRIVATE_LIST, Object_Ability);
    for (n = (NODE *)LIST_HEAD(ObjectListPrivate); NODE_SUCC(n) && (count < 8); n = (NODE *)NODE_SUCC(n))
    {
        abilityId[count] = OL_NR(n);
        need[count] = tcToolRequirement(toolId, OL_NR(n));
        count++;
    }
    RemoveList(ObjectListPrivate);

    if (!count) return; /* anyone can use it */

    CreateNode(desc, 0L, "");
    for (i = 0; i < count; i++)
    {
        dbGetObjectName(abilityId[i], name);
        snprintf(line, sizeof(line), toolNeeds[lang], name, (unsigned)((need[i] * 100 + 127) / 255));
        CreateNode(desc, 0L, line);
    }

    joined_byAll(Person_Matt_Stuvysunt, OLF_PRIVATE_LIST | OLF_INCLUDE_NAME, Object_Person);
    gang = ObjectListPrivate;
    for (n = (NODE *)LIST_HEAD(gang); NODE_SUCC(n); n = (NODE *)NODE_SUCC(n))
    {
        uint32_t persId = OL_NR(n);
        uint32_t worst = 255;
        int ok = 1;

        for (i = 0; i < count; i++)
        {
            uint32_t skill = has(persId, abilityId[i]) ? hasGet(persId, abilityId[i]) : 0;
            if (skill < need[i]) ok = 0;
            if (skill < worst) worst = skill;
        }
        if (ok)
        {
            snprintf(line, sizeof(line), toolCan[lang], NODE_NAME(n), (unsigned)((worst * 100 + 127) / 255));
            CreateNode(desc, 0L, line);
            users++;
        }
    }
    RemoveList(gang);

    if (!users) CreateNode(desc, 0L, (char *)toolNobody[lang]);
}

ubyte tcDescTool(ubyte choice)
{
    LIST *tools = NULL;
    LIST *desc = NULL;
    char line[TXT_KEY_LENGTH];
    ubyte oldChoice = 0;
    char exp[TXT_KEY_LENGTH];
    Person mary = (Person)dbGetObject(Person_Mary_Bolton);

    ObjectListSuccString = tcShowPriceOfTool;
    ObjectListWidth = 48;

    hasAll(Person_Mary_Bolton,
           OLF_ALIGNED | OLF_PRIVATE_LIST | OLF_INCLUDE_NAME | OLF_INSERT_STAR | OLF_ADD_SUCC_STRING, Object_Tool);
    tools = ObjectListPrivate;

    ObjectListWidth = 0;
    ObjectListSuccString = NULL;

    txtGetFirstLine(BUSINESS_TXT, "THANKS", exp);
    ExpandObjectList(tools, exp);

    choice = min(choice, (GetNrOfNodes(tools) - 1));

    while (choice != GET_OUT)
    {
        SetPictID(MATT_PICTID);

        oldChoice = choice;

        if (ChoiceOk(choice = Bubble(tools, choice, 0L, 0L), GET_OUT, tools))
        {
            dbGetObjectName(OL_NR(GetNthNode(tools, (uint32_t)choice)), line);

            desc = txtGoKey(TOOLS_TXT, line);
            if (GameMode == GAME_MODE_PLUS) tcAddToolUsers(desc, OL_NR(GetNthNode(tools, (uint32_t)choice)));

            SetPictID(mary->PictID);
            Bubble(desc, 0, 0L, 0L);

            RemoveList(desc);
        }
        else
            choice = GET_OUT;
    }

    RemoveList(tools);

    return oldChoice;
}

ubyte tcShowTool(ubyte choice)
{
    LIST *tools = NULL;
    NODE *node = NULL;
    uint32_t toolID = 0;
    ubyte oldChoice = 0;
    char exp[TXT_KEY_LENGTH];

    ObjectListSuccString = tcShowPriceOfTool;
    ObjectListWidth = 48;

    hasAll(Person_Mary_Bolton,
           OLF_ALIGNED | OLF_PRIVATE_LIST | OLF_INCLUDE_NAME | OLF_INSERT_STAR | OLF_ADD_SUCC_STRING, Object_Tool);
    tools = ObjectListPrivate;

    ObjectListSuccString = NULL;
    ObjectListWidth = 0;

    txtGetFirstLine(BUSINESS_TXT, "THANKS", exp);
    ExpandObjectList(tools, exp);

    choice = min(choice, (GetNrOfNodes(tools) - 1));

    while (choice != GET_OUT)
    {
        SetPictID(MATT_PICTID);

        oldChoice = choice;

        if (ChoiceOk(choice = Bubble(tools, choice, 0L, 0L), GET_OUT, tools))
        {
            node = (NODE *)GetNthNode(tools, (uint32_t)choice);
            toolID = OL_NR(node);

            Present(toolID, "Tool", InitToolPresent);
        }
        else
            choice = GET_OUT;
    }

    RemoveList(tools);

    return oldChoice;
}

void tcSellTool()
{
    LIST *bubble = NULL;
    LIST *tools = NULL;
    NODE *node = NULL;
    uint32_t toolID = 0;
    uint32_t price = 0;
    ubyte choice = 0;
    Tool tool = NULL;
    Person mary = NULL;

    mary = (Person)dbGetObject(Person_Mary_Bolton);

    hasAll(Person_Matt_Stuvysunt, OLF_PRIVATE_LIST | OLF_INCLUDE_NAME | OLF_INSERT_STAR, Object_Tool);
    tools = ObjectListPrivate;

    dbRemObjectNode(tools, Tool_Hand);
    dbRemObjectNode(tools, Tool_Fusz);

    if (LIST_EMPTY(tools)) Say(BUSINESS_TXT, 0, MATT_PICTID, "MATT_HAS_NO_TOOL");

    while ((choice != GET_OUT) && (!LIST_EMPTY(tools)))
    {
        char exp[TXT_KEY_LENGTH];

        txtGetFirstLine(BUSINESS_TXT, "NO_CHOICE", exp);
        ExpandObjectList(tools, exp);

        SetPictID(MATT_PICTID);
        if (ChoiceOk(choice = Bubble(tools, 0, NULL, 0L), GET_OUT, tools))
        {
            ubyte choice2 = 0;

            node = (NODE *)GetNthNode(tools, (uint32_t)choice);
            toolID = OL_NR(node);

            tool = (Tool)dbGetObject(toolID);
            price = tcGetToolTraderOffer(tool);

            bubble = txtGoKeyAndInsert(BUSINESS_TXT, "ANGEBOT_WERKZ", price);

            SetPictID(mary->PictID);
            Bubble(bubble, 0, 0L, 0L);
            RemoveList(bubble);

            bubble = txtGoKey(BUSINESS_TXT, "VERKAUF");

            if (ChoiceOk(choice2 = Bubble(bubble, choice2, 0L, 0L), GET_OUT, bubble))
            {
                if (choice2 == 0)
                {
                    if (GamePlayMode & GP_MORE_MONEY)
                    {  // Lucy 2017-11-08 new cheat
                        if (price > 0) price = (price * 3) / 2;
                    }
                    tcAddPlayerMoney(price);
                    hasSet(Person_Mary_Bolton, toolID);
                    hasUnSet(Person_Matt_Stuvysunt, toolID);
                }
            }

            RemoveList(bubble);
        }
        else
            choice = GET_OUT;

        RemoveList(tools);

        hasAll(Person_Matt_Stuvysunt, OLF_PRIVATE_LIST | OLF_INCLUDE_NAME | OLF_INSERT_STAR, Object_Tool);
        tools = ObjectListPrivate;

        dbRemObjectNode(tools, Tool_Hand);
        dbRemObjectNode(tools, Tool_Fusz);
    }

    RemoveList(tools);
}

void tcToolsShop(void)
{
    ubyte choice = 0;
    static ubyte choice1 = 0;

    if (!(knows(Person_Matt_Stuvysunt, Person_Mary_Bolton))) knowsSet(Person_Matt_Stuvysunt, Person_Mary_Bolton);

    while (choice != 4)
    {
        choice = Say(BUSINESS_TXT, choice, MATT_PICTID, "Tools Shop");

        switch (choice)
        {
            case 0:
                choice1 = tcBuyTool(choice1);
                break;
            case 1:
                tcSellTool();
                break;
            case 2:
                choice1 = tcShowTool(choice1);
                break;
            case 3:
                choice1 = tcDescTool(choice1);
                break;
            default:
                choice = 4;
                break;
        }

        if (choice1 == GET_OUT) choice1 = 0;
    }
}
