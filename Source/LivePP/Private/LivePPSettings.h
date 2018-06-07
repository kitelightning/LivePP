#include "Engine/DeveloperSettings.h"
#include "LivePPSettings.generated.h"


UENUM()
enum class ELPPSyncPointLocation : uint8
{
    None,
    EngineBeginFrame,
    EngineEndFrame,
};

UENUM(meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor="true"))
enum class ELPPHookFilter : uint8
{
    None        = 0,
    Game        = 1,
    GameProject = 2,
    CoreEngine  = 4,
    CoreEditor  = 8,

    CustomList = 128,
    All        = 255,
};
ENUM_CLASS_FLAGS(ELPPHookFilter);

UCLASS(config=EditorPerProjectUserSettings, meta=(DisplayName="LivePP"))
class ULivePPSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    //Runtime toggling of enabling that gets saved to per user project settings
    UPROPERTY(config, EditAnywhere, Category=LivePP, meta=(ConfigRestartRequired=true))
    bool bEnable;

    //TODO: ikrimae: #ThirdParty-LivePP: Test more extensively
    UPROPERTY(config, EditAnywhere, Category=LivePP, meta=(ConfigRestartRequired=true, Bitmask, BitmaskEnum="ELPPHookFilter"))
    int32 moduleFilter;

    UPROPERTY(config, EditAnywhere, Category=LivePP, meta=(ConfigRestartRequired=true))
    bool  bHookImports;

    UPROPERTY(config, EditAnywhere, Category=LivePP, meta=(ConfigRestartRequired=true))
    bool  bUseAsyncModuleLoad;

    UPROPERTY(config, EditAnywhere, Category=LivePP, meta=(ConfigRestartRequired=true))
    ELPPSyncPointLocation lppHotReloadSyncPoint;

    UPROPERTY(config, EditAnywhere, Category=LivePP, meta=(ConfigRestartRequired=true))
    TArray<FName> CoreEngineModuleNames;

    UPROPERTY(config, EditAnywhere, Category=LivePP, meta=(ConfigRestartRequired=true))
    TArray<FName> CoreEditorModuleNames;

    UPROPERTY(config, EditAnywhere, Category=LivePP, meta=(ConfigRestartRequired=true))
    TArray<FName> CustomModuleNames;

    ULivePPSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
    {
        CategoryName          = TEXT("Plugins");
        SectionName           = TEXT("LivePP");

        bEnable               = false;
        moduleFilter          = int32(ELPPHookFilter::Game | ELPPHookFilter::GameProject /*| ELPPHookFilter::CoreEngine | ELPPHookFilter::CoreEditor | ELPPHookFilter::All*/);
        bHookImports          = true;
        bUseAsyncModuleLoad   = true;
        lppHotReloadSyncPoint = ELPPSyncPointLocation::EngineEndFrame;

        CoreEngineModuleNames = {
            FName("Core"),
            FName("CoreUObject"),
            FName("Engine"),
            FName("RenderCore"),
            FName("ShaderCore"),
            FName("RHI"),
            FName("InputCore"),
        };

        CoreEditorModuleNames = {
            FName("UnrealEd"),
            FName("Sequencer"),
        };

        CustomModuleNames = {
            FName("Slate"),
            FName("SlateCore"),
            FName("ApplicationCore")
        };
    }

    virtual FName GetContainerName() const override
    {
        static const FName projectName(TEXT("Project"));
        return projectName;
    }

    virtual void PostInitProperties() override
    {
        Super::PostInitProperties();

#if WITH_EDITOR
        if (IsTemplate())
        {
            ImportConsoleVariableValues();
        }
#endif
    }

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override
    {
        Super::PostEditChangeProperty(PropertyChangedEvent);

        if (PropertyChangedEvent.Property)
        {
            ExportValuesToConsoleVariables(PropertyChangedEvent.Property);
        }
    }
#endif
};
