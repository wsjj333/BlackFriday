// #include "Character/Pusher/BFPusherLegacy.h"
//
// #include "Components/CapsuleComponent.h"
// #include "Character/Common/BFCharacterAnimInstance.h"
// #include "Character/Pusher/Components/BFCharacterAppearanceComponent.h"
// #include "Character/Pusher/Components/BFPusherDriveComponent.h"
// #include "Character/Pusher/Components/BFPusherInputComponent.h"
// #include "Vehicle/Cart/BFCartMovementComponent.h"
// #include "Vehicle/Cart/BFCartPawn.h"
//
// ABFPusherLegacy::ABFPusherLegacy()
// {
// 	bReplicates = true;
// 	
// 	CartDrivingComp = CreateDefaultSubobject<UBFCartMovementComponent>(TEXT("CartDrivingComp"));
// 	PusherInputComp = CreateDefaultSubobject<UBFPusherInputComponent>(TEXT("PusherInputComp"));
// 	PusherDriveComp = CreateDefaultSubobject<UBFPusherDriveComponent>(TEXT("PusherDriveComp"));
// 	AppearanceComp = CreateDefaultSubobject<UBFCharacterAppearanceComponent>(TEXT("AppearanceComp"));
//
// 	GetCapsuleComponent()->SetCapsuleHalfHeight(110.0f);
//
// 	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -110.0f));
// 	GetMesh()->SetRelativeRotation(FRotator(0.0f, 0.0f, -90.0f));
// }
//
// void ABFPusherLegacy::PawnClientRestart()
// {
// 	Super::PawnClientRestart();
// 	
// 	if (PusherInputComp)
// 	{
// 		PusherInputComp->EnsureMappingContext();
// 	}
// }
//
// void ABFPusherLegacy::BeginPlay()
// {
// 	Super::BeginPlay();
// 	
// 	if (AppearanceComp)
// 	{
// 		AppearanceComp->OnAppearanceApplied.AddDynamic(this, &ABFPusherLegacy::HandleAppearanceApplied);
// 	}
//
// 	RefreshAnimInstanceCache();
// }
//
// void ABFPusherLegacy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
// {
// 	Super::SetupPlayerInputComponent(PlayerInputComponent);
// 	
// 	if (!IsLocallyControlled())
// 	{
// 		return;
// 	}
//
// 	if (PusherInputComp)
// 	{
// 		PusherInputComp->BindInput(PlayerInputComponent);
// 	}
// }
//
// void ABFPusherLegacy::RefreshAnimInstanceCache()
// {
// 	if (USkeletalMeshComponent* MeshComp = GetMesh())
// 	{
// 		UAnimInstance* Current = MeshComp->GetAnimInstance();
// 		if (CachedAnimInstance != Current)
// 		{
// 			CachedAnimInstance = Cast<UBFCharacterAnimInstance>(Current);
// 		}
// 	}
// }
//
// void ABFPusherLegacy::HandleAppearanceApplied(EBFCharacterType AppliedType)
// {
// 	// 메시 교체 후 AnimInstance가 재생성/교체될 수 있으니 캐시 갱신
// 	RefreshAnimInstanceCache();
// }
//
// void ABFPusherLegacy::Tick(float DeltaSeconds)
// {
// 	Super::Tick(DeltaSeconds);
//
// 	// IK는 “복제된 Cart의 Transform” 기반으로 각자 계산해도 OK
// 	if (!PusherDriveComp->IsDriving() || !GetCart() || !CachedAnimInstance)
// 	{
// 		return;
// 	}
//
// 	USkeletalMeshComponent* MeshComp = GetMesh();
// 	if (!MeshComp) return;
//
// 	const FTransform HandleL_WS = GetCart()->GetHandleLTransform();
// 	const FTransform HandleR_WS = GetCart()->GetHandleRTransform();
//
// 	const FTransform HandleL_CS = HandleL_WS.GetRelativeTransform(MeshComp->GetComponentTransform());
// 	const FTransform HandleR_CS = HandleR_WS.GetRelativeTransform(MeshComp->GetComponentTransform());
//
// 	CachedAnimInstance->HandleTargetL_CS = HandleL_CS;
// 	CachedAnimInstance->HandleTargetR_CS = HandleR_CS;
// }
//
// bool ABFPusherLegacy::IsDriving() const
// {
// 	return PusherDriveComp ? PusherDriveComp->IsDriving() : false;
// }
//
// ABFCartPawn* ABFPusherLegacy::GetCart() const
// {
// 	return PusherDriveComp ? PusherDriveComp->GetCart() : nullptr;
// }
//
// void ABFPusherLegacy::SetCart(ABFCartPawn* NewCart) const
// {
// 	if (PusherDriveComp) PusherDriveComp->SetCart(NewCart);
// }
