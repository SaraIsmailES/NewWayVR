// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CharacterNewWay.generated.h"

UCLASS()
class NEWWAYVR_API ACharacterNewWay : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ACharacterNewWay();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
private:
	bool FindTeleportDestination(FVector& OutLocation);// HELPER
	void UpdateDestinationMarker();
	void UpdateBlinkers();
	FVector2D GetBlinkerCenter();

	void MoveForward(float Value);
	void MoveRight(float Value);

	void BeginTeleport();
	void FinishTeleport();
	void StartFade(float FromAlpha, float ToAlpha);// HELPER

private:
	UPROPERTY()
		class UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere)
		class UMotionControllerComponent* LeftController;
	UPROPERTY(VisibleAnywhere)
		class UMotionControllerComponent* RightController;

	UPROPERTY()
		class USceneComponent* VRRoot;

	UPROPERTY(VisibleAnywhere)
		class USplineComponent* TeleportPath;

	UPROPERTY(VisibleAnywhere)
		class UStaticMeshComponent* DestinationMarker;

	UPROPERTY()
		class UPostProcessComponent* PostProcessComponent;

	UPROPERTY(EditAnywhere)
		class UMaterialInterface* BlinkerMaterialBase;

	UPROPERTY()
		class UMaterialInstanceDynamic* BlinkerMaterialInstance;

	UPROPERTY(EditAnywhere)
		class UCurveFloat* RadiusVsVelocity;

private:
	UPROPERTY(EditAnywhere)
		float TeleportProjectileRadius = 10;

	UPROPERTY(EditAnywhere)
		float MaxTeleportSpeed = 800;

	UPROPERTY(EditAnywhere)
		float TeleportSimulationTime = 1;

	UPROPERTY(EditAnywhere)
		float TeleportFadeTime = 1.0f;

	UPROPERTY(EditAnywhere)
		FVector TeleportProjectionExtent = FVector(100, 100, 100);
};
