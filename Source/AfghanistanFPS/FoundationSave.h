#pragma once
#include "Kismet/GameplayStatics.h"
#include "GameFramework/SaveGame.h"
#include "Serialization/MemoryReader.h"
#include "Misc/Crc.h"

namespace FoundationSave
{
    // Leave the UE payload intact for editor inspection and append integrity
    // metadata. The marker and size also distinguish this trailer from data.
    inline constexpr uint32 Marker=0x31465341;
    inline constexpr int32 Limit=8*1024*1024;
    inline bool Write(USaveGame* Object,const FString& Slot)
    {
        TArray<uint8> Data;
        if(!UGameplayStatics::SaveGameToMemory(Object,Data) || Data.Num()>Limit-12)return false;
        const uint32 Trailer[]={Marker,uint32(Data.Num()),FCrc::MemCrc32(Data.GetData(),Data.Num())};
        Data.Append(reinterpret_cast<const uint8*>(Trailer),sizeof(Trailer));
        return UGameplayStatics::SaveDataToSlot(Data,Slot,0);
    }
    template<class T> T* Read(const FString& Slot)
    {
        TArray<uint8> Data;
        if(!UGameplayStatics::LoadDataFromSlot(Data,Slot,0) || Data.Num()<64 || Data.Num()>Limit)return nullptr;
        uint32 Trailer[3];const int32 Payload=Data.Num()-sizeof(Trailer);
        FMemory::Memcpy(Trailer,Data.GetData()+Payload,sizeof(Trailer));
        if(Trailer[0]==Marker)
        {
            if(Trailer[1]!=uint32(Payload) || Trailer[2]!=FCrc::MemCrc32(Data.GetData(),Payload))return nullptr;
            Data.SetNum(Payload);
        }
        // Pre-trailer saves from this engine build are recognized using a
        // known-good class header and the property stream's terminal FName.
        TArray<uint8> Reference;
        if(!UGameplayStatics::SaveGameToMemory(GetMutableDefault<T>(),Reference))return nullptr;
        const int64 Header=UGameplayStatics::StripSaveGameHeader(Reference).Tell();
        if(Header<=0 || Reference.Num()<Header+9 || Data.Num()<Header+9 ||
            FMemory::Memcmp(Data.GetData(),Reference.GetData(),Header)!=0 ||
            FMemory::Memcmp(Data.GetData()+Data.Num()-9,Reference.GetData()+Reference.Num()-9,9)!=0)return nullptr;
        return Cast<T>(UGameplayStatics::LoadGameFromMemory(Data));
    }
}
