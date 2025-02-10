#include "BeansTestUtilities.h"
#include "BeansWidgetPositioning.h"
#include "Kismet/GameplayStatics.h"
#define SPEC_BASE_NAME "BeansWidget.BeansWidgetTests"
BEGIN_DEFINE_SPEC(FBeansWidgetSpec, "BeansWidget",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask |
	EAutomationTestFlags::ClientContext)

const FVector2D ViewportSize = FVector2D(100,100);
const FVector2D WidgetResolution = FVector2D(10,10);
FVector2D MousePosition;
float Tolerance;

END_DEFINE_SPEC(FBeansWidgetSpec)

void FBeansWidgetSpec::Define()
{
	Describe("A widget positioned with ViewportPositionBound()", [this]
	{
		Describe("With a 100% tolerance", [this]
		{
			BeforeEach([this]
			{
				Tolerance = 100.0f;
			});
			
			It("with mouse position at 0,0", [this]()
			{
				MousePosition = FVector2D(0.0f, 0.0f);
				FVector2D expectedResult = FVector2D(5.0f, 5.0f);
				FVector2D actualResult = UBeansWidgetPositioning::CalculateBounds(MousePosition, ViewportSize, WidgetResolution, 1.0f, Tolerance);
				TestNearlyEqual("Should get expected FVector", actualResult.X, expectedResult.X);
				TestNearlyEqual("Should get expected FVector", actualResult.Y, expectedResult.Y);
				return true;
			});

			It("with mouse position at 100,100", [this]()
			{
				MousePosition = FVector2D(100.0f, 100.0f);
				FVector2D expectedResult = FVector2D(95.0f, 95.0f);
				FVector2D actualResult = UBeansWidgetPositioning::CalculateBounds(MousePosition, ViewportSize, WidgetResolution, 1.0f, Tolerance);
				TestNearlyEqual("Should get expected FVector", actualResult.X, expectedResult.X);
				TestNearlyEqual("Should get expected FVector", actualResult.Y, expectedResult.Y);
				return true;
			});

			It("with mouse position at 0,100", [this]()
			{
				MousePosition = FVector2D(0.0f, 100.0f);
				FVector2D expectedResult = FVector2D(5.0f, 95.0f);
				FVector2D actualResult = UBeansWidgetPositioning::CalculateBounds(MousePosition, ViewportSize, WidgetResolution, 1.0f, Tolerance);
				TestNearlyEqual("Should get expected FVector", actualResult.X, expectedResult.X);
				TestNearlyEqual("Should get expected FVector", actualResult.Y, expectedResult.Y);
				return true;
			});

			It("with mouse position at 100,0", [this]()
			{
				MousePosition = FVector2D(100.0f, 0.0f);
				FVector2D expectedResult = FVector2D(95.0f, 5.0f);
				FVector2D actualResult = UBeansWidgetPositioning::CalculateBounds(MousePosition, ViewportSize, WidgetResolution, 1.0f, Tolerance);
				TestNearlyEqual("Should get expected FVector", actualResult.X, expectedResult.X);
				TestNearlyEqual("Should get expected FVector", actualResult.Y, expectedResult.Y);
				return true;
			});

			It("with mouse position at 50,50", [this]()
			{
				MousePosition = FVector2D(50.0f, 50.0f);
				FVector2D expectedResult = FVector2D(50.0f, 50.0f);
				FVector2D actualResult = UBeansWidgetPositioning::CalculateBounds(MousePosition, ViewportSize, WidgetResolution, 1.0f, Tolerance);
				TestNearlyEqual("Should get expected FVector", actualResult.X, expectedResult.X);
				TestNearlyEqual("Should get expected FVector", actualResult.Y, expectedResult.Y);
				return true;
			});
			
		});

		Describe("With a 60% tolerance", [this]
		{
			BeforeEach([this]
			{
				Tolerance = 60.0f;
			});
			
			It("with mouse position at 0,0", [this]()
			{
				MousePosition = FVector2D(0.0f, 0.0f);
				FVector2D expectedResult = FVector2D(3.0f, 3.0f);
				FVector2D actualResult = UBeansWidgetPositioning::CalculateBounds(MousePosition, ViewportSize, WidgetResolution, 1.0f, Tolerance);
				TestNearlyEqual("Should get expected FVector", actualResult.X, expectedResult.X);
				TestNearlyEqual("Should get expected FVector", actualResult.Y, expectedResult.Y);
				return true;
			});

			It("with mouse position at 100,100", [this]()
			{
				MousePosition = FVector2D(100.0f, 100.0f);
				FVector2D expectedResult = FVector2D(97.0f, 97.0f);
				FVector2D actualResult = UBeansWidgetPositioning::CalculateBounds(MousePosition, ViewportSize, WidgetResolution, 1.0f, Tolerance);
				TestNearlyEqual("Should get expected FVector", actualResult.X, expectedResult.X);
				TestNearlyEqual("Should get expected FVector", actualResult.Y, expectedResult.Y);
				return true;
			});

			It("with mouse position at 0,100", [this]()
			{
				MousePosition = FVector2D(0.0f, 100.0f);
				FVector2D expectedResult = FVector2D(3.0f, 97.0f);
				FVector2D actualResult = UBeansWidgetPositioning::CalculateBounds(MousePosition, ViewportSize, WidgetResolution, 1.0f, Tolerance);
				TestNearlyEqual("Should get expected FVector", actualResult.X, expectedResult.X);
				TestNearlyEqual("Should get expected FVector", actualResult.Y, expectedResult.Y);
				return true;
			});

			It("with mouse position at 100,0", [this]()
			{
				MousePosition = FVector2D(100.0f, 0.0f);
				FVector2D expectedResult = FVector2D(97.0f, 3.0f);
				FVector2D actualResult = UBeansWidgetPositioning::CalculateBounds(MousePosition, ViewportSize, WidgetResolution, 1.0f, Tolerance);
				TestNearlyEqual("Should get expected FVector", actualResult.X, expectedResult.X);
				TestNearlyEqual("Should get expected FVector", actualResult.Y, expectedResult.Y);
				return true;
			});

			It("with mouse position at 50,50", [this]()
			{
				MousePosition = FVector2D(50.0f, 50.0f);
				FVector2D expectedResult = FVector2D(50.0f, 50.0f);
				FVector2D actualResult = UBeansWidgetPositioning::CalculateBounds(MousePosition, ViewportSize, WidgetResolution, 1.0f, Tolerance);
				TestNearlyEqual("Should get expected FVector", actualResult.X, expectedResult.X);
				TestNearlyEqual("Should get expected FVector", actualResult.Y, expectedResult.Y);
				return true;
			});
		});
	});
}
