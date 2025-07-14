// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerChar.h"

// Sets default values
APlayerChar::APlayerChar()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//Initial setup of camera component
	PlayerCamComp = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Cam"));

	//Attaching camera to character mesh and head bone
	PlayerCamComp->SetupAttachment(GetMesh(), "head");

	//Shares rotation with controller
	PlayerCamComp->bUsePawnControlRotation = true;


	//Setting the number of elements in the array
	BuildingArray.SetNum(3);
	ResourcesArray.SetNum(3);
	//Setting each element in the name array
	ResourcesNameArray.Add(TEXT("Wood"));
	ResourcesNameArray.Add(TEXT("Stone"));
	ResourcesNameArray.Add(TEXT("Berry"));

}

// Called when the game starts or when spawned
void APlayerChar::BeginPlay()
{
	Super::BeginPlay();
	
	//Establish F Timer Handle
	FTimerHandle StatsTimerHandle;
	//Timer will tick every 2 seconds
	GetWorld()->GetTimerManager().SetTimer(StatsTimerHandle, this, &APlayerChar::DecreaseStats, 2.0f, true);
}

// Called every frame
void APlayerChar::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//if building is active and spawned part is a valid object, line trace will be set up to allow us to place object
	//Object will follow wherever we're pointing at until we click again
	if (isBuilding)
	{
		if (spawnedPart)
		{
			FVector StartLocation = PlayerCamComp->GetComponentLocation();
			FVector Direction = PlayerCamComp->GetForwardVector() * 400.0f;
			FVector EndLocation = StartLocation + Direction;
			spawnedPart->SetActorLocation(EndLocation);
		}
	}

}

// Called to bind functionality to input
void APlayerChar::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	//Setup axis and action inputs
	PlayerInputComponent->BindAxis("MoveForward", this, &APlayerChar::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &APlayerChar::MoveRight);
	PlayerInputComponent->BindAxis("LookUp", this, &APlayerChar::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn", this, &APlayerChar::AddControllerYawInput);
	PlayerInputComponent->BindAction("JumpEvent", IE_Pressed, this, &APlayerChar::StartJump);
	PlayerInputComponent->BindAction("JumpEvent", IE_Released, this, &APlayerChar::StopJump);

	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &APlayerChar::FindObject);
	PlayerInputComponent->BindAction("RotPart", IE_Pressed, this, &APlayerChar::RotateBulding);
}

//Defined functions for player movement
void APlayerChar::MoveForward(float axisValue)
{
	FVector Direction = FRotationMatrix(Controller->GetControlRotation()).GetScaledAxis(EAxis::X);
	AddMovementInput(Direction, axisValue);
}

void APlayerChar::MoveRight(float axisValue)
{
	FVector Direction = FRotationMatrix(Controller->GetControlRotation()).GetScaledAxis(EAxis::Y);
	AddMovementInput(Direction, axisValue);
}

void APlayerChar::StartJump()
{
	bPressedJump = true;
}

void APlayerChar::StopJump()
{
	bPressedJump = false;
}

void APlayerChar::FindObject()
{
	//Location of camera + direction camera is facing(800 units ahead of camera)
	FHitResult HitResult;
	FVector StartLocation = PlayerCamComp->GetComponentLocation();
	FVector Direction = PlayerCamComp->GetForwardVector() * 800.0f;
	FVector EndLocation = StartLocation + Direction;

	FCollisionQueryParams QueryParams;
	//Line trace ignores player character
	QueryParams.AddIgnoredActor(this);
	//Allows us to see complex collisions
	QueryParams.bTraceComplex = true;
	QueryParams.bReturnFaceIndex = true;

	//If isBuilding is not active, line trace can happen
	if (!isBuilding)
	{
		//Line trace
		if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, QueryParams))
		{
			AResource_M* HitResource = Cast<AResource_M>(HitResult.GetActor());

			if (Stamina > 5.0f)
			{
				//Validation to see if HitResource matches AResource_M
				if (HitResource)
				{
					FString hitName = HitResource->resourceName;
					int resourceValue = HitResource->resourceAmount;

					HitResource->totalResource = HitResource->totalResource - resourceValue;

					if (HitResource->totalResource > resourceValue)
					{
						GiveResource(resourceValue, hitName);

						//Print message to show that resource is collected
						check(GEngine != nullptr);
						GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Resource Collected"));

						UGameplayStatics::SpawnDecalAtLocation(GetWorld(), hitDecal, FVector(10.0f, 10.0f, 10.0f), HitResult.Location, FRotator(-90, 0, 0), 2.0f);

						SetStamina(-5.0f);
					}
					else
					{
						//Destroy hit result is there is no more resources left
						HitResource->Destroy();
						//Print message to show that resource is depleted
						check(GEngine != nullptr);
						GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Resource Depleted"));
					}
				}
			}
		}
	}

	else
	{
		isBuilding = false;
	}
}


//Functions to adjust health, hunger, and stamina variables
void APlayerChar::SetHealth(float amount)
{
	if (Health + amount < 100)
	{
		Health = Health + amount;
	}
}

void APlayerChar::SetHunger(float amount)
{
	if (Hunger + amount < 100)
	{
		Hunger = Hunger + amount;
	}
}

void APlayerChar::SetStamina(float amount)
{
	if (Stamina + amount <= 100)
	{
		Stamina = Stamina + amount;
	}
}

//Function to adjust variables when called
void APlayerChar::DecreaseStats()
{
	//Decreases hunger by 1 every time function gets called if hunger is greater than 0
	if (Hunger > 0)
	{
		SetHunger(-1.0f);
	}

	//Stamina regeneration
	SetStamina(10.0f);

	//Decreases health is hunger is at or below 0
	if (Hunger <= 0)
	{
		SetHealth(-3.0f);
	}
}

//Determines which resource the player is hitting and adding the amount to the array in their respective index
void APlayerChar::GiveResource(float amount, FString resourceType)
{
	if (resourceType == "Wood")
	{
		ResourcesArray[0] = ResourcesArray[0] + amount;
	}

	if (resourceType == "Stone")
	{
		ResourcesArray[1] = ResourcesArray[1] + amount;
	}

	if (resourceType == "Berry")
	{
		ResourcesArray[2] = ResourcesArray[2] + amount;
	}
}

void APlayerChar::UpdateResources(float woodAmount, float stoneAmount, FString buildingObject)
{
	//subtracting the amount of resources it takes to build an object
	if (woodAmount <= ResourcesArray[0])
	{
		if (stoneAmount <= ResourcesArray[1])
		{
			ResourcesArray[0] = ResourcesArray[0] - woodAmount;
			ResourcesArray[1] = ResourcesArray[1] - stoneAmount;

			if (buildingObject == "Wall")
			{
				BuildingArray[0] = BuildingArray[0] + 1;
			}

			if (buildingObject == "Floor")
			{
				BuildingArray[1] = BuildingArray[1] + 1;
			}

			if (buildingObject == "Ceiling")
			{
				BuildingArray[2] = BuildingArray[2] + 1;
			}
		}
	}
}

void APlayerChar::SpawnBuilding(int buildingID, bool& isSuccess)
{
	//if player is not building, check if BuildingArray has a particular object
	if (!isBuilding)
	{
		if (BuildingArray[buildingID] >= 1)
		{
			isBuilding = true;
			FActorSpawnParameters SpawnParams;
			FVector StartLocation = PlayerCamComp->GetComponentLocation();
			FVector Direction = PlayerCamComp->GetForwardVector() * 400.0f;
			FVector EndLocation = StartLocation + Direction;
			FRotator myRot(0, 0, 0);

			//subtracts 1 from BuildingArray if we're building a particular object
			BuildingArray[buildingID] = BuildingArray[buildingID] - 1;

			spawnedPart = GetWorld()->SpawnActor<ABuildingPart>(BuildPartClass, EndLocation, myRot, SpawnParams);

			isSuccess = true;
		}

		isSuccess = false;

	}
}

void APlayerChar::RotateBulding()
{
	if (isBuilding)
	{
		spawnedPart->AddActorWorldRotation(FRotator(0, 90, 0));
	}
}

