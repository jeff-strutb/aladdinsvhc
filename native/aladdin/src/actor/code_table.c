/*
 * code_table.c: every function an actor record can run in, by name (made by tools/gen_code_table.py; its --check
 * fails when a row of a kinds_*.c table or an ActorRunAs names a function not here). The snapshot writes
 * Actor.code as the hash of the name, which holds across builds.
 */
#include "actor/actor_own.h"
#include "actor/code_table.h"

/* each declared here again (a repeated declaration is the same one): the headers that declare them are many */
int ActorBriefAnim8(Actor *a);
int ActorBriefAnimB(Actor *a);
int ChainPieceStart(Actor *a);
int ContinueChoiceStart(Actor *a);
int Kind08Start(Actor *a);
int Kind12AtPlayer(Actor *a);
int Kind20Start(Actor *a);
int Kind21Start(Actor *a);
int Kind22Start(Actor *a);
int Kind23Code(Actor *a);
int Kind24Start(Actor *a);
int Kind25Code(Actor *a);
int Kind27Start(Actor *a);
int Kind28Start(Actor *a);
int Kind2AStart(Actor *a);
int Kind2BCode(Actor *a);
int Kind2CCode(Actor *a);
int Kind2DCode(Actor *a);
int Kind2EStart(Actor *a);
int Kind2FCode(Actor *a);
int Kind30Start(Actor *a);
int Kind31Start(Actor *a);
int Kind33Code(Actor *a);
int Kind34Start(Actor *a);
int Kind35Start(Actor *a);
int Kind36End(Actor *a);
int Kind36Start(Actor *a);
int Kind38Start(Actor *a);
int Kind3ACode(Actor *a);
int Kind3BStart(Actor *a);
int Kind3CDrift(Actor *a);
int Kind3DCode(Actor *a);
int Kind3ECode(Actor *a);
int Kind3FStart(Actor *a);
int Kind40Start(Actor *a);
int Kind41Code(Actor *a);
int Kind42Start(Actor *a);
int Kind43Start(Actor *a);
int Kind44Start(Actor *a);
int Kind46Start(Actor *a);
int Kind48Start(Actor *a);
int Kind49Carry(Actor *a);
int Kind4AShow(Actor *a);
int Kind4BStart(Actor *a);
int Kind4CStart(Actor *a);
int Kind4DSecond(Actor *a);
int Kind4DStart(Actor *a);
int Kind4EStart(Actor *a);
int Kind4FCode(Actor *a);
int Kind50Code(Actor *a);
int Kind51Start(Actor *a);
int Kind52Start(Actor *a);
int Kind53Held(Actor *a);
int Kind54Start(Actor *a);
int Kind55Start(Actor *a);
int Kind56Start(Actor *a);
int Kind57Start(Actor *a);
int Kind58Start(Actor *a);
int Kind59Code(Actor *a);
int Kind5AStart(Actor *a);
int Kind5BStart(Actor *a);
int Kind5CStart(Actor *a);
int Kind5DStart(Actor *a);
int Kind5EBrief(Actor *a);
int Kind5FSecond(Actor *a);
int Kind5FStart(Actor *a);
int Kind60Start(Actor *a);
int Kind61Second(Actor *a);
int Kind61Start(Actor *a);
int Kind62Second(Actor *a);
int Kind63Code(Actor *a);
int Kind64Second(Actor *a);
int Kind64Start(Actor *a);
int Kind65Start(Actor *a);
int Kind66Second(Actor *a);
int Kind66Start(Actor *a);
int Kind67Second(Actor *a);
int Kind67Start(Actor *a);
int Kind68Code(Actor *a);
int Kind69Start(Actor *a);
int Kind6AStart(Actor *a);
int Kind6BCode(Actor *a);
int Kind6CSecond(Actor *a);
int Kind6CStart(Actor *a);
int Kind6DStart(Actor *a);
int Kind6EStart(Actor *a);
int Kind70Second(Actor *a);
int Kind70Start(Actor *a);
int Kind71Start(Actor *a);
int Kind72Code(Actor *a);
int Kind73Start(Actor *a);
int Kind76Second(Actor *a);
int Kind76Start(Actor *a);
int Kind77Second(Actor *a);
int Kind77Start(Actor *a);
int Kind78Start(Actor *a);
int Kind79Rise(Actor *a);
int Kind7AFly(Actor *a);
int Kind7BStart(Actor *a);
int Kind7CStart(Actor *a);
int Kind7DKeepSpot(Actor *a);
int Kind7ECode(Actor *a);
int Kind7FCode(Actor *a);
int Kind81Code(Actor *a);
int Kind82Show(Actor *a);
int Kind83Code(Actor *a);
int Kind84Start(Actor *a);
int Kind85Start(Actor *a);
int Kind86Start(Actor *a);
int Kind87Second(Actor *a);
int Kind87Start(Actor *a);
int Kind88Start(Actor *a);
int Kind89Start(Actor *a);
int Kind8BStart(Actor *a);
int Kind8CStart(Actor *a);
int Kind8DCode(Actor *a);
int Kind8ECode(Actor *a);
int Kind8FWalk(Actor *a);
int Kind90Code(Actor *a);
int Kind91Start(Actor *a);
int Kind92Start(Actor *a);
int Kind93Code(Actor *a);
int Kind94Second(Actor *a);
int Kind94Spark(Actor *a);
int Kind95Start(Actor *a);
int Kind97Start(Actor *a);
int Kind98Start(Actor *a);
int Kind99Start(Actor *a);
int Kind9AStart(Actor *a);
int Kind9BStory(Actor *a);
int Kind9CStart(Actor *a);
int Kind9EStart(Actor *a);
int KindA0Start(Actor *a);
int KindA1Start(Actor *a);
int KindA2Sent(Actor *a);
int KindA3Shot(Actor *a);
int KindA4Start(Actor *a);
int KindA5Start(Actor *a);
int KindA6Stay(Actor *a);
int KindA7Start(Actor *a);
int KindA8Start(Actor *a);
int LandingPad(Actor *a);
int PlatformCourseStart(Actor *a);
int PlayerCode(Actor *a);
int ShotCode(Actor *a);
int SteppedShowActor(Actor *a);

const ActorCodeEntry actor_code_table[] = {
    { "ActorBriefAnim8", ActorBriefAnim8 },
    { "ActorBriefAnimB", ActorBriefAnimB },
    { "ChainPieceStart", ChainPieceStart },
    { "ContinueChoiceStart", ContinueChoiceStart },
    { "Kind08Start", Kind08Start },
    { "Kind12AtPlayer", Kind12AtPlayer },
    { "Kind20Start", Kind20Start },
    { "Kind21Start", Kind21Start },
    { "Kind22Start", Kind22Start },
    { "Kind23Code", Kind23Code },
    { "Kind24Start", Kind24Start },
    { "Kind25Code", Kind25Code },
    { "Kind27Start", Kind27Start },
    { "Kind28Start", Kind28Start },
    { "Kind2AStart", Kind2AStart },
    { "Kind2BCode", Kind2BCode },
    { "Kind2CCode", Kind2CCode },
    { "Kind2DCode", Kind2DCode },
    { "Kind2EStart", Kind2EStart },
    { "Kind2FCode", Kind2FCode },
    { "Kind30Start", Kind30Start },
    { "Kind31Start", Kind31Start },
    { "Kind33Code", Kind33Code },
    { "Kind34Start", Kind34Start },
    { "Kind35Start", Kind35Start },
    { "Kind36End", Kind36End },
    { "Kind36Start", Kind36Start },
    { "Kind38Start", Kind38Start },
    { "Kind3ACode", Kind3ACode },
    { "Kind3BStart", Kind3BStart },
    { "Kind3CDrift", Kind3CDrift },
    { "Kind3DCode", Kind3DCode },
    { "Kind3ECode", Kind3ECode },
    { "Kind3FStart", Kind3FStart },
    { "Kind40Start", Kind40Start },
    { "Kind41Code", Kind41Code },
    { "Kind42Start", Kind42Start },
    { "Kind43Start", Kind43Start },
    { "Kind44Start", Kind44Start },
    { "Kind46Start", Kind46Start },
    { "Kind48Start", Kind48Start },
    { "Kind49Carry", Kind49Carry },
    { "Kind4AShow", Kind4AShow },
    { "Kind4BStart", Kind4BStart },
    { "Kind4CStart", Kind4CStart },
    { "Kind4DSecond", Kind4DSecond },
    { "Kind4DStart", Kind4DStart },
    { "Kind4EStart", Kind4EStart },
    { "Kind4FCode", Kind4FCode },
    { "Kind50Code", Kind50Code },
    { "Kind51Start", Kind51Start },
    { "Kind52Start", Kind52Start },
    { "Kind53Held", Kind53Held },
    { "Kind54Start", Kind54Start },
    { "Kind55Start", Kind55Start },
    { "Kind56Start", Kind56Start },
    { "Kind57Start", Kind57Start },
    { "Kind58Start", Kind58Start },
    { "Kind59Code", Kind59Code },
    { "Kind5AStart", Kind5AStart },
    { "Kind5BStart", Kind5BStart },
    { "Kind5CStart", Kind5CStart },
    { "Kind5DStart", Kind5DStart },
    { "Kind5EBrief", Kind5EBrief },
    { "Kind5FSecond", Kind5FSecond },
    { "Kind5FStart", Kind5FStart },
    { "Kind60Start", Kind60Start },
    { "Kind61Second", Kind61Second },
    { "Kind61Start", Kind61Start },
    { "Kind62Second", Kind62Second },
    { "Kind63Code", Kind63Code },
    { "Kind64Second", Kind64Second },
    { "Kind64Start", Kind64Start },
    { "Kind65Start", Kind65Start },
    { "Kind66Second", Kind66Second },
    { "Kind66Start", Kind66Start },
    { "Kind67Second", Kind67Second },
    { "Kind67Start", Kind67Start },
    { "Kind68Code", Kind68Code },
    { "Kind69Start", Kind69Start },
    { "Kind6AStart", Kind6AStart },
    { "Kind6BCode", Kind6BCode },
    { "Kind6CSecond", Kind6CSecond },
    { "Kind6CStart", Kind6CStart },
    { "Kind6DStart", Kind6DStart },
    { "Kind6EStart", Kind6EStart },
    { "Kind70Second", Kind70Second },
    { "Kind70Start", Kind70Start },
    { "Kind71Start", Kind71Start },
    { "Kind72Code", Kind72Code },
    { "Kind73Start", Kind73Start },
    { "Kind76Second", Kind76Second },
    { "Kind76Start", Kind76Start },
    { "Kind77Second", Kind77Second },
    { "Kind77Start", Kind77Start },
    { "Kind78Start", Kind78Start },
    { "Kind79Rise", Kind79Rise },
    { "Kind7AFly", Kind7AFly },
    { "Kind7BStart", Kind7BStart },
    { "Kind7CStart", Kind7CStart },
    { "Kind7DKeepSpot", Kind7DKeepSpot },
    { "Kind7ECode", Kind7ECode },
    { "Kind7FCode", Kind7FCode },
    { "Kind81Code", Kind81Code },
    { "Kind82Show", Kind82Show },
    { "Kind83Code", Kind83Code },
    { "Kind84Start", Kind84Start },
    { "Kind85Start", Kind85Start },
    { "Kind86Start", Kind86Start },
    { "Kind87Second", Kind87Second },
    { "Kind87Start", Kind87Start },
    { "Kind88Start", Kind88Start },
    { "Kind89Start", Kind89Start },
    { "Kind8BStart", Kind8BStart },
    { "Kind8CStart", Kind8CStart },
    { "Kind8DCode", Kind8DCode },
    { "Kind8ECode", Kind8ECode },
    { "Kind8FWalk", Kind8FWalk },
    { "Kind90Code", Kind90Code },
    { "Kind91Start", Kind91Start },
    { "Kind92Start", Kind92Start },
    { "Kind93Code", Kind93Code },
    { "Kind94Second", Kind94Second },
    { "Kind94Spark", Kind94Spark },
    { "Kind95Start", Kind95Start },
    { "Kind97Start", Kind97Start },
    { "Kind98Start", Kind98Start },
    { "Kind99Start", Kind99Start },
    { "Kind9AStart", Kind9AStart },
    { "Kind9BStory", Kind9BStory },
    { "Kind9CStart", Kind9CStart },
    { "Kind9EStart", Kind9EStart },
    { "KindA0Start", KindA0Start },
    { "KindA1Start", KindA1Start },
    { "KindA2Sent", KindA2Sent },
    { "KindA3Shot", KindA3Shot },
    { "KindA4Start", KindA4Start },
    { "KindA5Start", KindA5Start },
    { "KindA6Stay", KindA6Stay },
    { "KindA7Start", KindA7Start },
    { "KindA8Start", KindA8Start },
    { "LandingPad", LandingPad },
    { "PlatformCourseStart", PlatformCourseStart },
    { "PlayerCode", PlayerCode },
    { "ShotCode", ShotCode },
    { "SteppedShowActor", SteppedShowActor },
    { 0, 0 }
};
