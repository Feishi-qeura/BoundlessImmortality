#pragma once
#include "CoreMinimal.h"
#include "InkFrontendTypes.h"

/** Presentation only: ranks and entitlement are provided by the game, never inferred from gifts here. */
namespace InkLeaderboard
{
    constexpr int32 RowCount = 30;
    constexpr int32 VisibleRows = 8;
    constexpr int32 AnchorVisibleRows = 5;
    inline bool SameEntry(const FInkLeaderboardEntry& A,const FInkLeaderboardEntry& B)
    {
        return A.ParticipantId==B.ParticipantId&&A.DisplayName.EqualTo(B.DisplayName)&&A.Category==B.Category&&A.Rank==B.Rank&&
            A.Metric.EqualTo(B.Metric)&&A.NumericMetric==B.NumericMetric&&A.TitleId==B.TitleId&&A.Realm.EqualTo(B.Realm)&&A.RealmIndex==B.RealmIndex&&A.AvatarTexture==B.AvatarTexture&&A.AvatarUrl==B.AvatarUrl;
    }
    inline bool SameEntries(const TArray<FInkLeaderboardEntry>& A,const TArray<FInkLeaderboardEntry>& B)
    {if(A.Num()!=B.Num())return false;for(int32 I=0;I<A.Num();++I)if(!SameEntry(A[I],B[I]))return false;return true;}
    struct FRowLayout
    {
        float Width=774,Height=220;
        FVector4 Name,Metric,Title,Avatar,Medal,Realm,Number;
    };
    inline void AlignTierContent(FRowLayout& Row,int32 Rank,FName Category,bool Compact)
    {
        if(Rank<4||Rank>10)return;
        // Top, bottom and crop height measured in original atlas pixels. Decorations
        // extend above the writing surface, so centering against the whole row is wrong.
        static const FVector3f Sword[]={{32,163,170},{17,145,155},{18,145,151},{13,136,145},{11,127,136},{12,122,128},{11,116,123}};
        static const FVector3f Speed[]={{9,152,163},{8,143,151},{9,148,152},{10,139,151},{7,133,138},{10,130,138},{8,129,134}};
        static const FVector3f Damage[]={{30,161,167},{25,155,161},{21,148,154},{23,145,151},{19,139,143},{26,141,152},{42,149,154}};
        const auto& Band=(Category==TEXT("Damage")?Damage:Category==TEXT("CultivationSpeed")?Speed:Sword)[Rank-4];
        // RowBackground paints at y=2 with a four-unit outer gap. Keep artwork,
        // row height, list spacing and horizontal columns exactly as authored.
        const float Padding=Compact?1.f:2.f,ArtHeight=Row.Height-4.f;
        const float Top=2.f+ArtHeight*Band.X/Band.Z+Padding;
        const float Bottom=2.f+ArtHeight*Band.Y/Band.Z-Padding;
        const float OldTop=Row.Name.Y;
        const float ContentHeight=FMath::Max(Row.Title.Y+Row.Title.W,Row.Realm.Y+Row.Realm.W)-OldTop;
        const float Scale=FMath::Min(1.f,(Bottom-Top)/FMath::Max(1.f,ContentHeight));
        const float NewTop=(Top+Bottom-ContentHeight*Scale)*.5f;
        for(FVector4* Rect:{&Row.Name,&Row.Metric,&Row.Title,&Row.Realm,&Row.Avatar,&Row.Number,&Row.Medal})
        {Rect->Y=NewTop+(Rect->Y-OldTop)*Scale;Rect->W*=Scale;}
    }
    inline FRowLayout FullRowLayout(bool Portrait,int32 Rank=4,FName Category=TEXT("Anchor"))
    {
        FRowLayout Row;
        // Taller podium art needs its own row, but ordinary tiers should not inherit its whitespace.
        Row.Width=Portrait?624:800;
        Row.Height=Portrait?(Rank==1?128:Rank==2?112:Rank==3?104:Rank<=10?96:84):
            (Rank==1?160:Rank==2?140:Rank==3?132:Rank<=10?116:84);
        const float EmblemClearance=Portrait&&Category==TEXT("Anchor")?(Rank==1?16.f:Rank==2?10.f:0.f):0.f;
        const float Top=(Row.Height-58)*.5f+(Rank<=3?4.f:0.f)+EmblemClearance;
        if(Portrait)
        {
            Row.Name=FVector4(204,Top,132,28);Row.Metric=FVector4(344,Top,140,28);
            Row.Title=FVector4(204,Top+30,132,26);Row.Realm=FVector4(344,Top+30,140,26);
            Row.Avatar=FVector4(164,Top+9,32,32);Row.Number=FVector4(132,Top+11,26,28);Row.Medal=Row.Number;
        }
        else
        {
            Row.Name=FVector4(280,Top,140,28);Row.Realm=FVector4(430,Top+30,140,26);
            Row.Title=FVector4(280,Top+30,140,26);Row.Metric=FVector4(430,Top,140,28);
            Row.Avatar=FVector4(236,Top+9,36,36);Row.Number=FVector4(204,Top+13,26,28);Row.Medal=Row.Number;
        }
        return Row;
    }
    inline FRowLayout CompactRowLayout(int32 Rank=4)
    {
        FRowLayout Row;
        Row.Width=324;Row.Height=Rank==1?64:Rank==2?58:Rank==3?54:48;
        const float Top=(Row.Height-34)*.5f;
        Row.Name=FVector4(96,Top,76,16);Row.Metric=FVector4(176,Top,60,16);
        Row.Title=FVector4(96,Top+19,76,15);Row.Realm=FVector4(176,Top+19,60,15);
        Row.Avatar=FVector4(70,Top+6,22,22);Row.Number=FVector4(46,Top+7,20,20);Row.Medal=Row.Number;
        return Row;
    }

    inline TArray<FInkLeaderboardEntry> MakeRows(const TArray<FInkLeaderboardEntry>& Entries, FName Category)
    {
        TArray<FInkLeaderboardEntry> Rows;
        Rows.SetNum(RowCount);
        TSet<int32> Occupied;
        for(int32 I=0; I<RowCount; ++I){Rows[I].Rank=I+1;Rows[I].Category=Category;}
        for(const auto& Entry:Entries)
        {
            if(Entry.Category!=Category || Entry.Rank<1 || Entry.Rank>RowCount || Occupied.Contains(Entry.Rank))continue;
            Rows[Entry.Rank-1]=Entry;
            Occupied.Add(Entry.Rank);
        }
        return Rows;
    }

    struct FFadeState
    {
        FName Category;
        float Progress=0;
        float DamageRemaining=0;

        float Opacity()const{return Progress*Progress*(3.f-2.f*Progress);}
        void TickRequested(float Delta,bool bHUD,FName Desired,float FadeIn,float FadeOut)
        {
            Delta=FMath::Max(0.f,Delta);
            if(!bHUD){Category=NAME_None;Progress=0;DamageRemaining=0;return;}
            if(Category.IsNone()&&!Desired.IsNone())Category=Desired;
            if(Category==Desired&&!Desired.IsNone())Progress=FMath::Min(1.f,Progress+Delta/FMath::Max(.01f,FadeIn));
            else {Progress=FMath::Max(0.f,Progress-Delta/FMath::Max(.01f,FadeOut));if(Progress==0)Category=Desired;}
        }
        void Tick(float Delta,bool bHUD,bool bCultivating,bool bAttacking,float FadeIn,float FadeOut,float Hold)
        {
            Delta=FMath::Max(0.f,Delta);
            if(!bHUD){Category=NAME_None;Progress=0;DamageRemaining=0;return;}
            DamageRemaining=bAttacking?FMath::Max(0.f,Hold):FMath::Max(0.f,DamageRemaining-Delta);
            const FName Desired=bAttacking||DamageRemaining>0?FName(TEXT("Damage")):bCultivating?FName(TEXT("CultivationSpeed")):NAME_None;
            if(Category.IsNone() && !Desired.IsNone())Category=Desired;
            if(Category==Desired && !Desired.IsNone())Progress=FMath::Min(1.f,Progress+Delta/FMath::Max(.01f,FadeIn));
            else
            {
                Progress=FMath::Max(0.f,Progress-Delta/FMath::Max(.01f,FadeOut));
                if(Progress==0)Category=Desired;
            }
        }
    };
}
