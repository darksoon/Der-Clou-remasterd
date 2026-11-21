/*  _________  _______
   / ___/ __ \/ __/ _ \      Der Clou!
  / /__/ /_/ /\ \/ ___/ Open Source Project
  \___/\____/___/_/ http://cosp.sourceforge.net
   Based on the original by neo Software GmbH
*/
#include "planing/io.h"

#include "port/port.h"

struct IOData
{
    NODE io_Link;
    ubyte io_Data;
};

static char *Planing_Open[3] = {"r", "w", "r"};

// Loading & saving functions
void plSaveTools(FILE *fh)
{
    if (fh)
    {
        struct ObjectNode *n = NULL;
        hasAll(Person_Matt_Stuvysunt, OLF_NORMAL, Object_Tool);

        fprintf(fh, PLANING_PLAN_TOOL_BEGIN_ID "\n");
        for (n = (struct ObjectNode *)LIST_HEAD(ObjectList); NODE_SUCC(n); n = (struct ObjectNode *)NODE_SUCC(n))
        {
            fprintf(fh, "%u\n", OL_NR(n));
        }
        fprintf(fh, PLANING_PLAN_TOOL_END_ID "\n");
    }
}

LIST *plLoadTools(FILE *fh)
{
    LIST *l = txtGoKey(PLAN_TXT, "SYSTEM_TOOLS_MISSING_1");
    ubyte foundAll = 1;
    ubyte canGet = 2;
    ubyte toolsNr = 0;
    char buffer[32];

    buffer[0] = '\0';

    if (fh)
    {
        while (dskGets(buffer, 31, fh))
        {
            if (!strcmp(buffer, PLANING_PLAN_TOOL_END_ID)) break;

            if (atol(buffer))
            {
                toolsNr++;

                if (!has(Person_Matt_Stuvysunt, atol(buffer)))
                {
                    if (has(Person_Mary_Bolton, atol(buffer)))
                    {
                        canGet++;
                        dbAddObjectNode(l, atol(buffer), OLF_INCLUDE_NAME);
                    }

                    foundAll = 0;
                }
            }
        }
    }

    if (foundAll)
    {
        RemoveList(l);
        l = NULL;
    }
    else
    {
        LIST *extList = NULL;
        NODE *n = NULL;
        if (canGet == 2)
        {
            extList = txtGoKey(PLAN_TXT, "SYSTEM_TOOLS_MISSING_3");
        }
        else
        {
            if ((toolsNr - canGet) > 1)
            {
                extList = txtGoKeyAndInsert(PLAN_TXT, "SYSTEM_TOOLS_MISSING_2", (uint32_t)(toolsNr - canGet));
            }
            else if (toolsNr - canGet)
            {
                extList = txtGoKey(PLAN_TXT, "SYSTEM_TOOLS_MISSING_4");
            }
        }
        if (extList)
        {
            for (n = LIST_HEAD(extList); NODE_SUCC(n); n = (NODE *)NODE_SUCC(n))
            {
                CreateNode(l, 0, NODE_NAME(n));
            }
            RemoveList(extList);
        }
    }
    return l;
}

/* ------------------------------------------------------------------------
 * Plans are files per building and do not know the mode they were made in. Game+ can lower
 * the skill a tool needs (PLUS/BALANCE.LUA), and the planner only checks the skill when the
 * action is planned. So a plan made in Game+ could be loaded in an original game with a tool
 * action the rules of 1994 refuse. In the original mode every tool action of a loaded plan is
 * checked again, the same way the planner does it; a plan made in the original mode always passes.
 * ------------------------------------------------------------------------ */

/* texts in the game font: \x84 = ae, \x81 = ue, \x82 = e acute */
static const char *plAbilityMissing[4][2] = {
    {"This plan asks for skills my team does not", "have. That goes for..."},
    {"F\x81r diesen Plan reichen die F\x84higkeiten im", "Team nicht aus. Das betrifft..."},
    {"Ce plan demande des capacit\x82s que mon", "\x82quipe n'a pas. Cela concerne..."},
    {"Este plan exige habilidades que mi equipo", "no tiene. Se trata de..."}};

static LIST *plAbilityList = NULL; /* burglars whose saved tool actions fail the check */

static ubyte plAbilityListed(uint32_t persId)
{
    struct ObjectNode *n = NULL;

    for (n = (struct ObjectNode *)LIST_HEAD(plAbilityList); NODE_SUCC(n); n = (struct ObjectNode *)NODE_SUCC(n))
        if (OL_NR(n) == persId) return 1;
    return 0;
}

static void plCheckSavedUse(uint32_t persId, uint32_t itemId, uint32_t toolId)
{
    LIST *requires = NULL;
    NODE *n = NULL;
    ubyte ok = 1;

    /* stairs carry an area and guards are fought without a skill value: only real tool work counts */
    if (!persId || !toolId || !itemId) return;
    if (!dbIsObject(persId, Object_Person) || !dbIsObject(toolId, Object_Tool)) return;
    if (dbIsObject(itemId, Object_Police)) return;

    toolRequiresAll(toolId, OLF_PRIVATE_LIST, Object_Ability);
    requires = ObjectListPrivate;

    for (n = LIST_HEAD(requires); NODE_SUCC(n); n = (NODE *)NODE_SUCC(n))
    {
        if (!has(persId, OL_NR(n)) || (hasGet(persId, OL_NR(n)) < tcToolRequirement(toolId, OL_NR(n))))
        {
            ok = 0;
            break;
        }
    }

    RemoveList(requires);

    if (!ok && !plAbilityListed(persId))
    {
        char name[TXT_KEY_LENGTH];

        dbGetObjectName(toolId, name);
        Log("Plan refused: burglar %u lacks the skill for %s", (unsigned)persId, name);
        dbAddObjectNode(plAbilityList, persId, OLF_INCLUDE_NAME);
    }
}

/* NULL if the plan fits, otherwise the message to show (like LoadSystem and plLoadTools) */
LIST *plCheckPlanAbilities(FILE *fh, uint32_t objId)
{
    LIST *l = NULL;
    ubyte lang = txtGetLanguage();

    /* Game+ made the plan under its own rules; the plan of the barracks is part of the story */
    if (!fh || (GameMode == GAME_MODE_PLUS) || (objId == Building_Starford_Kaserne)) return NULL;
    if (GamePlayMode & (GP_GUARD_DESIGN | GP_LEVEL_DESIGN)) return NULL;

    if (lang > 3) lang = 1;

    plAbilityList = (LIST *)CreateList(0L);
    ScanSavedUseActions(fh, plCheckSavedUse);

    if (LIST_EMPTY(plAbilityList))
    {
        RemoveList(plAbilityList);
    }
    else
    {
        struct ObjectNode *n = NULL;

        l = (LIST *)CreateList(0L);
        CreateNode(l, 0L, (char *)plAbilityMissing[lang][0]);
        CreateNode(l, 0L, (char *)plAbilityMissing[lang][1]);
        for (n = (struct ObjectNode *)LIST_HEAD(plAbilityList); NODE_SUCC(n); n = (struct ObjectNode *)NODE_SUCC(n))
            dbAddObjectNode(l, OL_NR(n), OLF_INCLUDE_NAME);
        RemoveList(plAbilityList);
    }

    plAbilityList = NULL;
    return l;
}

ubyte plOpen(uint32_t objId, ubyte mode, FILE **fh)
{
    if (GamePlayMode & GP_GUARD_DESIGN)
    {
        if (grdInit(fh, Planing_Open[mode], objId, lsGetActivAreaID())) return PLANING_OPEN_OK;
    }
    else
    {
        LIST *PlanList = NULL;
        ubyte i = 0;
        FILE *pllFh = NULL;
        uint32_t pllData = 0L;
        char pllPath[DSK_PATH_MAX];
        char pllUserPath[DSK_PATH_MAX];
        char name1[DSK_PATH_MAX];
        char name2[TXT_KEY_LENGTH];
        char expan[TXT_KEY_LENGTH];
        struct IOData *ioData = NULL;

        dbGetObjectName(lsGetActivAreaID(), name1);
        name1[strlen(name1) - 1] = '\0';  // remove number

        // MOD 25-04-94 HG - new paths on pc
        sprintf(name2, "%s%s", name1, PLANING_PLAN_LIST_EXTENSION);

        dskBuildPathNameUser(DATADISK_DIRECTORY, name2, pllPath);
        dskBuildPathNameUser(DATADISK_DIRECTORY, name2, pllUserPath);
        if (dskFileLength(pllPath) == 0)
        {
            dskBuildPathName(DATADISK_DIRECTORY, name2, pllPath);
        }

        if ((pllFh = dskOpen(pllPath, "r", PLANING_PLAN_DISK)))
        {
            dskGets(name2, TXT_KEY_LENGTH - 1, pllFh);
            pllData = atol(name2);
            dskClose(pllFh);

            if ((mode == PLANING_OPEN_WRITE_PLAN) || pllData)
            {
                sprintf(name2, "MODE_%d", mode);
                plMessage(name2, PLANING_MSG_REFRESH);

                PlanList = (LIST *)CreateList(0L);

                dbGetObjectName(objId, name1);

                expan[0] = EOS;

                for (i = 0; i < PLANING_NR_PLANS; i++)
                {
                    if ((mode == PLANING_OPEN_WRITE_PLAN) || (pllData & (1L << i)))
                    {
                        if (mode == PLANING_OPEN_WRITE_PLAN)
                        {
                            if (pllData & (1L << i))
                                txtGetFirstLine(PLAN_TXT, "ATTENTION_1", expan);
                            else
                                txtGetFirstLine(PLAN_TXT, "ATTENTION_2", expan);
                        }

                        sprintf(name2, "*%s Plan %d    %s", name1, i + 1, expan);
                        ioData = (struct IOData *)CreateNode(PlanList, sizeof(struct IOData), name2);
                        ioData->io_Data = i;
                    }
                }

                sprintf(name2, "EXPAND_MODE_%d", mode);
                txtGetFirstLine(PLAN_TXT, name2, expan);
                ExpandObjectList(PlanList, expan);

                i = Bubble(PlanList, 0, NULL, 0L);

                if (ChoiceOk(i, GET_OUT, PlanList))
                {
                    ioData = (struct IOData *)GetNthNode(PlanList, i);
                    i = ioData->io_Data;

                    dbGetObjectName(lsGetActivAreaID(), name1);
                    name1[strlen(name1) - 1] = '\0';

                    sprintf(name2, "%s%d%s", name1, i + 1, PLANING_PLAN_EXTENSION);

                    dskBuildPathNameUser(DATADISK_DIRECTORY, name2, name1);

                    *fh = dskOpen(name1, Planing_Open[mode], PLANING_PLAN_DISK);

                    if (mode == PLANING_OPEN_WRITE_PLAN)
                    {
                        pllData |= 1L << i;
                        if ((pllFh = dskOpen(pllUserPath, "w", PLANING_PLAN_DISK)))
                        {
                            fprintf(pllFh, "%u", pllData);
                            dskClose(pllFh);
                        }
                    }

                    if (*fh)
                    {
                        RemoveList(PlanList);
                        return PLANING_OPEN_OK;
                    }
                    else
                        NewErrorMsg(Disk_Defect, __FILE__, __func__, 0);
                }

                RemoveList(PlanList);

                return PLANING_OPEN_ERR_NO_CHOICE;
            }

            return PLANING_OPEN_ERR_NO_PLAN;
        }
    }

    NewErrorMsg(Disk_Defect, __FILE__, __func__, 0);
    return 0;
}

void plSave(uint32_t objId)
{
    FILE *fh = NULL;

    if (plOpen(objId, PLANING_OPEN_WRITE_PLAN, &fh) == PLANING_OPEN_OK)
    {
        if (GamePlayMode & GP_GUARD_DESIGN)
        {
            grdDo(fh, plSys, PersonsList, BurglarsNr, PersonsNr, GUARDS_DO_SAVE);
        }
        else
        {
            ubyte i = 0;

            SaveSystem(fh, plSys);
            plSaveTools(fh);

            for (i = 0; i < BurglarsNr; i++) SaveHandler(fh, plSys, OL_NR(GetNthNode(PersonsList, i)));
        }

        dskClose(fh);
        dskSyncUserData(); /* persist plan + plan list in the browser build */
    }
}

void plSaveChanged(uint32_t objId)
{
    if (PlanChanged)
    {
        LIST *l = txtGoKey(PLAN_TXT, "PLAN_CHANGED");

        inpTurnESC(0);

        if (Bubble(l, 0, NULL, 0L) == 0)
        {
            inpTurnESC(1);

            if (!plAllInCar(objId)) plSay("PLAN_NOT_FINISHED", 0);

            plSave(objId);
        }
        else
            inpTurnESC(1);

        RemoveList(l);
    }
}

void plLoad(uint32_t objId)
{
    FILE *fh = NULL;
    ubyte ret = 0;

    if (objId == Building_Starford_Kaserne)
    {
        while ((ret = plOpen(objId, PLANING_OPEN_READ_PLAN, &fh)) != PLANING_OPEN_OK)
            ;
    }
    else
    {
        ret = plOpen(objId, PLANING_OPEN_READ_PLAN, &fh);
    }

    if (ret == PLANING_OPEN_OK)
    {
        if (GamePlayMode & GP_GUARD_DESIGN)
        {
            grdDo(fh, plSys, PersonsList, BurglarsNr, PersonsNr, GUARDS_DO_LOAD);
        }
        else
        {
            LIST *l = NULL;
            ubyte i = 0;
            ubyte goon = 1;

            if ((l = LoadSystem(fh, plSys)))
            {
                inpTurnESC(0);
                Bubble(l, 0, NULL, 0L);
                inpTurnESC(1);
                RemoveList(l);

                goon = 0;
                l = NULL;
            }

            if ((l = plLoadTools(fh)))
            {
                inpTurnESC(0);
                Bubble(l, 0, NULL, 0L);
                inpTurnESC(1);
                RemoveList(l);

                goon = 0;
            }

            if (goon && (l = plCheckPlanAbilities(fh, objId)))
            {
                inpTurnESC(0);
                Bubble(l, 0, NULL, 0L);
                inpTurnESC(1);
                RemoveList(l);

                goon = 0;
            }

            if (goon)
            {
                for (i = 0; i < BurglarsNr; i++) LoadHandler(fh, plSys, OL_NR(GetNthNode(PersonsList, i)));
            }
        }

        dskClose(fh);
    }
    else
    {
        if (ret == PLANING_OPEN_ERR_NO_PLAN) plMessage("NO_PLAN", PLANING_MSG_REFRESH | PLANING_MSG_WAIT);
    }
}
