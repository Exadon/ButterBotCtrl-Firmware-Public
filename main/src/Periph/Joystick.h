#ifndef BB_CTRL_CONTROLLER_JOYSTICK_H
#define BB_CTRL_CONTROLLER_JOYSTICK_H

#include <algorithm>
#include <atomic>
#include <hal/gpio_types.h>
#include <Entity/AsyncEntity.h>
#include <Memory/SmartPtr/StrongObjectPtr.h>
#include <Services/ADCReader.h>

class Joystick : public AsyncEntity {
	GENERATED_BODY(Joystick, AsyncEntity, CONSTRUCTOR_PACK(gpio_num_t, gpio_num_t))

public:
	Joystick(gpio_num_t hPin = GPIO_NUM_NC, gpio_num_t vPin = GPIO_NUM_NC, bool internalStack = true);

	int8_t getHorizontal() const noexcept;
	int8_t getVertical() const noexcept;

	/** Live readings for the diagnostics stream; safe from any thread. */
	struct Diag {
		int x = 0, y = 0;             // processed (-100..100, what menus / RC use)
		int rawX = 0, rawY = 0;       // filtered ADC counts (0..4095)
		int centerX = 0, centerY = 0; // calibrated centre in ADC counts
	};
	Diag getDiag() const noexcept;

protected:
	void tick(float deltaTime) noexcept override;

private:
	static constexpr TickType_t SampleInterval = 20; // ms
	static constexpr float EmaA = 0.3f;
	static constexpr int DeadZone = 15; // ±15 points around center
	static constexpr float MaxMapping = 65.0f; // After which points around center is it considered to be at 100%
	static constexpr int RawCenter = 2048;
	static constexpr int RawMax = 4095;

	/**
	 * Centre calibration. The stock code assumed the stick rests at exactly 2048, but real sticks rest
	 * off-centre; a resting position near the dead-zone edge then flickers in and out of "pushed", which
	 * makes menus jump and the robot creep. At boot (stick untouched) we average the first samples per axis
	 * and use that as the centre - unless they're noisy or implausibly far off, in which case the stick is
	 * probably being held, and we keep 2048.
	 */
	static constexpr int CalibSamples = 16;            // ~320 ms at 20 ms per sample
	static constexpr float MaxCenterOffset = 400.0f;   // raw counts (~10% of range)
	static constexpr float MaxCalibSpread = 80.0f;     // raw counts between min and max while calibrating

	struct AxisCalib {
		float sum = 0, min = 1e9f, max = -1e9f;
		float center = RawCenter;

		void add(float raw){
			sum += raw;
			min = std::min(min, raw);
			max = std::max(max, raw);
		}
	};

	AxisCalib hCalib, vCalib;
	int calibCount = 0;
	bool calibrated = false;

	StrongObjectPtr<EMA_ADCFilter> hEMAFilter;
	StrongObjectPtr<EMA_ADCFilter> vEMAFilter;
	StrongObjectPtr<ADCReader> hReader;
	StrongObjectPtr<ADCReader> vReader;

	std::atomic<int8_t> hValue{ 0 };
	std::atomic<int8_t> vValue{ 0 };
	std::atomic<int16_t> hRaw{ 0 };
	std::atomic<int16_t> vRaw{ 0 };

	void finishCalibration();
	static int8_t mapSample(float raw, float center) noexcept;
};

#endif //BB_CTRL_CONTROLLER_JOYSTICK_H
