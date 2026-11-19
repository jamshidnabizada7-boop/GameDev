#include "Components/TacticalWeaponComponent.h"
#include "Components/BallisticImpactComponent.h"
#include "Components/CQBTakedownComponent.h"
#include "Actors/TacticalShellCasing.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

UTacticalWeaponComponent::UTacticalWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UTacticalWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	for (const auto& Pair : DefaultLoadout)
	{
		if (Pair.Value)
		{
			FTacticalWeaponInstance Inst;
			Inst.Data = Pair.Value;
			Inst.CurrentAmmo = Pair.Value->MagazineCapacity;
			Inst.ReserveAmmo = Pair.Value->StartingReserveAmmo;
			Inst.bSuppressorAttached = Pair.Value->bStartsSuppressed;
			LoadoutInstances.Add(Pair.Key, Inst);
		}
	}

	EquipSlot(ETacticalWeaponSlot::Primary);
}

void UTacticalWeaponComponent::InitializeWeaponMesh(USkeletalMeshComponent* InWeaponMesh)
{
	WeaponMeshComp = InWeaponMesh;
	EquipSlot(ActiveSlot);
}

void UTacticalWeaponComponent::EquipSlot(ETacticalWeaponSlot Slot)
{
	if (!LoadoutInstances.Contains(Slot)) return;

	StopFire();
	bIsReloading = false;
	GetWorld()->GetTimerManager().ClearTimer(ReloadTimer);

	ActiveSlot = Slot;
	const FTacticalWeaponInstance& Inst = LoadoutInstances[ActiveSlot];
	if (WeaponMeshComp && Inst.Data && Inst.Data->WeaponMesh)
	{
		WeaponMeshComp->SetSkeletalMesh(Inst.Data->WeaponMesh);
	}

	OnWeaponSlotChanged.Broadcast(ActiveSlot);
	OnAmmoUpdated.Broadcast(Inst.CurrentAmmo, Inst.ReserveAmmo);
}

void UTacticalWeaponComponent::PickupAndReplaceWeapon(UTacticalWeaponDataAsset* NewWeaponData, int32 OverrideAmmo)
{
	if (!NewWeaponData) return;

	FTacticalWeaponInstance Inst;
	Inst.Data = NewWeaponData;
	Inst.CurrentAmmo = (OverrideAmmo >= 0) ? OverrideAmmo : NewWeaponData->MagazineCapacity;
	Inst.ReserveAmmo = NewWeaponData->StartingReserveAmmo;
	Inst.bSuppressorAttached = NewWeaponData->bStartsSuppressed;

	LoadoutInstances.Add(NewWeaponData->Slot, Inst);
	EquipSlot(NewWeaponData->Slot);
}

UTacticalWeaponDataAsset* UTacticalWeaponComponent::GetActiveWeaponData() const
{
	if (const FTacticalWeaponInstance* Inst = LoadoutInstances.Find(ActiveSlot))
	{
		return Inst->Data;
	}
	return nullptr;
}

void UTacticalWeaponComponent::StartFire()
{
	if (bIsReloading) return;
	UTacticalWeaponDataAsset* Data = GetActiveWeaponData();
	if (!Data) return;

	bIsFiring = true;
	FireSingleShot();

	if (Data->FireMode == ETacticalFireMode::FullAuto)
	{
		const float Interval = 60.0f / FMath::Max(60.0f, Data->FireRateRPM);
		GetWorld()->GetTimerManager().SetTimer(AutoFireTimer, this, &UTacticalWeaponComponent::FireSingleShot, Interval, true);
	}
}

void UTacticalWeaponComponent::StopFire()
{
	bIsFiring = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoFireTimer);
	}
}

void UTacticalWeaponComponent::SetAiming(bool bNewAiming)
{
	bIsAiming = bNewAiming;
}

void UTacticalWeaponComponent::StartReload()
{
	FTacticalWeaponInstance* Inst = LoadoutInstances.Find(ActiveSlot);
	if (!Inst || !Inst->Data || bIsReloading) return;
	if (Inst->CurrentAmmo >= Inst->Data->MagazineCapacity || Inst->ReserveAmmo <= 0) return;

	StopFire();
	bIsReloading = true;
	if (Inst->Data->ReloadSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Inst->Data->ReloadSound, GetOwner()->GetActorLocation());
	}

	GetWorld()->GetTimerManager().SetTimer(ReloadTimer, this, &UTacticalWeaponComponent::FinishReload, Inst->Data->ReloadDuration, false);
}

void UTacticalWeaponComponent::FinishReload()
{
	bIsReloading = false;
	FTacticalWeaponInstance* Inst = LoadoutInstances.Find(ActiveSlot);
	if (!Inst || !Inst->Data) return;

	const int32 Needed = Inst->Data->MagazineCapacity - Inst->CurrentAmmo;
	const int32 Taken = FMath::Min(Needed, Inst->ReserveAmmo);
	Inst->CurrentAmmo += Taken;
	Inst->ReserveAmmo -= Taken;
	OnAmmoUpdated.Broadcast(Inst->CurrentAmmo, Inst->ReserveAmmo);
}

void UTacticalWeaponComponent::ToggleSuppressor()
{
	FTacticalWeaponInstance* Inst = LoadoutInstances.Find(ActiveSlot);
	if (!Inst || !Inst->Data || !Inst->Data->bSupportsSuppressor) return;

	Inst->bSuppressorAttached = !Inst->bSuppressorAttached;
}

void UTacticalWeaponComponent::FireSingleShot()
{
	FTacticalWeaponInstance* Inst = LoadoutInstances.Find(ActiveSlot);
	if (!Inst || !Inst->Data || bIsReloading) return;

	if (Inst->Data->FireMode != ETacticalFireMode::MeleeSlash)
	{
		if (Inst->CurrentAmmo <= 0)
		{
			StopFire();
			StartReload();
			return;
		}
		Inst->CurrentAmmo--;
		OnAmmoUpdated.Broadcast(Inst->CurrentAmmo, Inst->ReserveAmmo);
	}

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar) return;

	const FVector EyeLoc = OwnerChar->GetPawnViewLocation();
	const FVector AimDir = OwnerChar->GetControlRotation().Vector();
	const float SpreadDeg = Inst->Data->BaseSpreadDegrees * (bIsAiming ? Inst->Data->ADSSpreadMultiplier : 1.0f);

	UBallisticImpactComponent* ImpactComp = OwnerChar->FindComponentByClass<UBallisticImpactComponent>();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TacticalShot), true, OwnerChar);

	for (int32 i = 0; i < FMath::Max(1, Inst->Data->PelletsPerShot); ++i)
	{
		const FVector ShotDir = FMath::VRandCone(AimDir, FMath::DegreesToRadians(SpreadDeg));
		const FVector TraceEnd = EyeLoc + ShotDir * Inst->Data->MaxRange;

		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLoc, TraceEnd, ECC_Visibility, Params))
		{
			if (ImpactComp)
			{
				ImpactComp->HandleBallisticHit(Hit, ShotDir, Inst->Data->BaseDamage, Inst->Data->RagdollDeathImpulse);
			}
		}
	}

	// Recoil camera kick + procedural mesh kickback
	OwnerChar->AddControllerPitchInput(-Inst->Data->VerticalRecoilPitch);
	OwnerChar->AddControllerYawInput(FMath::FRandRange(-Inst->Data->HorizontalRecoilYaw, Inst->Data->HorizontalRecoilYaw));
	RecoilKickOffset = Inst->Data->ProceduralKickbackCm;

	// Audio & Muzzle VFX
	USoundBase* FireSFX = (Inst->bSuppressorAttached && Inst->Data->SuppressedFireSound) ? Inst->Data->SuppressedFireSound : Inst->Data->FireSound;
	if (FireSFX)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSFX, OwnerChar->GetActorLocation());
	}

	if (WeaponMeshComp && Inst->Data->MuzzleFlashVFX && !Inst->bSuppressorAttached)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			Inst->Data->MuzzleFlashVFX, WeaponMeshComp, Inst->Data->MuzzleSocketName,
			FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
	}

	SpawnShellCasing();
}

void UTacticalWeaponComponent::SpawnShellCasing()
{
	UTacticalWeaponDataAsset* Data = GetActiveWeaponData();
	if (!Data || !Data->ShellCasingClass || !WeaponMeshComp) return;

	const FTransform EjectTransform = WeaponMeshComp->DoesSocketExist(Data->ShellEjectSocketName)
		? WeaponMeshComp->GetSocketTransform(Data->ShellEjectSocketName)
		: WeaponMeshComp->GetComponentTransform();

	ATacticalShellCasing* Shell = GetWorld()->SpawnActor<ATacticalShellCasing>(
		Data->ShellCasingClass, EjectTransform.GetLocation(), EjectTransform.Rotator());
	if (Shell)
	{
		const FVector RightDir = EjectTransform.GetRotation().GetRightVector();
		const FVector UpDir = EjectTransform.GetRotation().GetUpVector();
		const FVector LaunchVel = RightDir * FMath::FRandRange(180.f, 260.f) + UpDir * FMath::FRandRange(90.f, 150.f);
		Shell->LaunchShell(LaunchVel, FVector(FMath::FRandRange(400.f, 900.f), FMath::FRandRange(300.f, 700.f), 500.f));
	}
}

void UTacticalWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateProceduralWeaponRig(DeltaTime);
}

void UTacticalWeaponComponent::UpdateProceduralWeaponRig(float DeltaTime)
{
	if (!WeaponMeshComp) return;
	UTacticalWeaponDataAsset* Data = GetActiveWeaponData();
	if (!Data) return;

	ADSAlpha = FMath::FInterpTo(ADSAlpha, bIsAiming ? 1.0f : 0.0f, DeltaTime, 14.0f);
	RecoilKickOffset = FMath::FInterpTo(RecoilKickOffset, 0.0f, DeltaTime, 18.0f);

	bool bHoldingShield = false;
	if (UCQBTakedownComponent* CQB = GetOwner()->FindComponentByClass<UCQBTakedownComponent>())
	{
		bHoldingShield = CQB->IsHoldingHumanShield();
	}

	FVector TargetOffset = FMath::Lerp(Data->HipOffset, Data->ADSOffset, ADSAlpha);
	FRotator TargetRot = FRotator::ZeroRotator;

	if (bHoldingShield)
	{
		TargetOffset = Data->ShieldCantedOffset;
		TargetRot = Data->ShieldCantedRotation;
	}

	TargetOffset.X -= RecoilKickOffset;
	WeaponMeshComp->SetRelativeLocation(FMath::VInterpTo(WeaponMeshComp->GetRelativeLocation(), TargetOffset, DeltaTime, 18.0f));
	WeaponMeshComp->SetRelativeRotation(FMath::RInterpTo(WeaponMeshComp->GetRelativeRotation(), TargetRot, DeltaTime, 14.0f));
}
