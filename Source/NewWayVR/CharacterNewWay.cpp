// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterNewWay.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SplineComponent.h"

#include "MotionControllerComponent.h"
#include "Camera/CameraComponent.h"

#include "Kismet/GameplayStatics.h"//Library
#include "NavigationSystem.h"

#include "Curves/CurveFloat.h"//Assets
#include "Materials/MaterialInstanceDynamic.h"


// Sets default values
ACharacterNewWay::ACharacterNewWay()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot"));
	VRRoot->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VRRoot);

	LeftController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftController"));
	LeftController->SetupAttachment(VRRoot);
	LeftController->SetTrackingSource(EControllerHand::Left);
	LeftController->bDisplayDeviceModel = true;

	RightController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightController"));
	RightController->SetupAttachment(VRRoot);
	RightController->SetTrackingSource(EControllerHand::Right);
	RightController->bDisplayDeviceModel = true;

	TeleportPath = CreateDefaultSubobject<USplineComponent>(TEXT("TeleportPath"));
	TeleportPath->SetupAttachment(RightController);

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestinationMarker"));
	DestinationMarker->SetupAttachment(GetRootComponent());

	PostProcessComponent = CreateAbstractDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent"));
	PostProcessComponent->SetupAttachment(GetRootComponent());

}

void ACharacterNewWay::BeginPlay()
{
	Super::BeginPlay();
	DestinationMarker->SetVisibility(false);

	if (BlinkerMaterialBase != nullptr)
	{


		//this is wrong crashs.... BlinkerMaterialInstance->Create(BlinkerMaterialBase, this);// that is right... UMaterialInstanceDynamic::Create();

		BlinkerMaterialInstance = UMaterialInstanceDynamic::Create(BlinkerMaterialBase, this);
		PostProcessComponent->AddOrUpdateBlendable(BlinkerMaterialInstance);
	}
}

void ACharacterNewWay::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector NewCameraOffset = Camera->GetComponentLocation() - GetActorLocation();
	NewCameraOffset.Z = 0;
	AddActorWorldOffset(NewCameraOffset);
	VRRoot->AddWorldOffset(-NewCameraOffset);

	UpdateDestinationMarker();
	UpdateBlinkers();
}

void ACharacterNewWay::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("Forward"), this, &ACharacterNewWay::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("Right"), this, &ACharacterNewWay::MoveRight);

	PlayerInputComponent->BindAction(TEXT("Teleport"), IE_Released, this, &ACharacterNewWay::BeginTeleport);

}

void ACharacterNewWay::MoveForward(float Value)
{
	AddMovementInput(Camera->GetForwardVector(), Value);
}

void ACharacterNewWay::MoveRight(float Value)
{
	AddMovementInput(Camera->GetRightVector(), Value);
}

bool ACharacterNewWay::FindTeleportDestination(FVector& OutLocation)// // HELPER ofter REFACTOR
{

	FVector Start = RightController->GetComponentLocation();
	FVector Look = RightController->GetForwardVector();

	FPredictProjectilePathParams Params(
		TeleportProjectileRadius,
		Start,
		Look * MaxTeleportSpeed,
		TeleportSimulationTime,
		ECollisionChannel::ECC_Visibility,
		this
	);
	Params.DrawDebugType = EDrawDebugTrace::ForOneFrame;
	Params.bTraceComplex = true;

	FPredictProjectilePathResult Result;


	bool bHit = UGameplayStatics::PredictProjectilePath(this, Params, Result);

	if (!bHit) return false;/// Nice 


	FNavLocation NavLocation;
	/*UNavigationSystemV1* NavSys = Cast<UNavigationSystemV1>(GetWorld()->GetNavigationSystem());
	NavSys->ProjectPointToNavigation(HitResult.Location, NavLocation, TeleportProjectionExtent);*/
	bool bOnNavMesh = UNavigationSystemV1::GetCurrent(GetWorld())->ProjectPointToNavigation(Result.HitResult.Location, NavLocation, TeleportProjectionExtent); // newWay for my previous solution

	if (!bOnNavMesh) return false;/// Nice 

	OutLocation = NavLocation.Location;

	return true;// OR return bOnNavMesh && bHit ; 
}

void ACharacterNewWay::UpdateDestinationMarker()
{
	FVector Location;
	bool bHasDestination = FindTeleportDestination(Location);

	if (bHasDestination)
	{
		DestinationMarker->SetWorldLocation(Location);
		DestinationMarker->SetVisibility(true);
	}
	else
	{
		DestinationMarker->SetVisibility(false);
	}
}

void ACharacterNewWay::UpdateBlinkers()
{
	if (RadiusVsVelocity == nullptr) return;

	float Speed = GetVelocity().Size();
	float Radius = RadiusVsVelocity->GetFloatValue(Speed);
	BlinkerMaterialInstance->SetScalarParameterValue(TEXT("Radius"), Radius);

	FVector2D Center = GetBlinkerCenter();
	BlinkerMaterialInstance->SetVectorParameterValue(TEXT("Centre"), FLinearColor(Center.X, Center.Y, 0));
}

FVector2D ACharacterNewWay::GetBlinkerCenter()
{
	FVector MovementDirection = GetVelocity().GetSafeNormal();

	if (MovementDirection.IsNearlyZero())
	{
		return FVector2D(0.5, 0.5);
	}
	FVector WorldStationaryLocation;

	if (FVector::DotProduct(Camera->GetForwardVector(), MovementDirection) > 0)
	{
		WorldStationaryLocation = Camera->GetComponentLocation() + MovementDirection * 1000;
	}
	else
	{
		WorldStationaryLocation = Camera->GetComponentLocation() + MovementDirection * 1000;
	}

	FVector2D ScreenStationaryLocation;

	APlayerController* PC = Cast<APlayerController>(GetController());

	if (PC == nullptr)
	{
		return FVector2D(0.5, 0.5);
	}

	PC->ProjectWorldLocationToScreen(WorldStationaryLocation, ScreenStationaryLocation);
	int32 SizeX, SizeY;
	PC->GetViewportSize(SizeX, SizeY);
	ScreenStationaryLocation.X /= SizeX;
	ScreenStationaryLocation.Y /= SizeY;
	return ScreenStationaryLocation;
}

void ACharacterNewWay::StartFade(float FromAlpha, float ToAlpha)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC != nullptr)
	{
		PC->PlayerCameraManager->StartCameraFade(FromAlpha, ToAlpha, TeleportFadeTime, FLinearColor::Black);
	}
}

void ACharacterNewWay::BeginTeleport()
{
	StartFade(0, 1);

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ACharacterNewWay::FinishTeleport, TeleportFadeTime);
}

void ACharacterNewWay::FinishTeleport()
{
	FVector NewActorLocation = DestinationMarker->GetComponentLocation();
	NewActorLocation.Z += GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	SetActorLocation(NewActorLocation);
	StartFade(1, 0);
}

