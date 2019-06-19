#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AsyncTask.generated.h"

UINTERFACE(MinimalAPI)
class UAsyncTask : public UInterface
{
	GENERATED_BODY()
};

class ACCOUNTTEST_API IAsyncTask
{
	GENERATED_BODY()

	
public:
	/* This event is called using CallTaskAsynchronously. */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void OnAsyncTask();
};
