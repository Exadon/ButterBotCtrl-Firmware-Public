#include "Joystick.h"
#include <algorithm>
#include <cmath>
#include <esp_log.h>
#include "Memory/ObjectMemory.h"

static const char* TAG = "Joystick";

Joystick::Joystick(gpio_num_t hPin, gpio_num_t vPin, bool internalStack) :
		Super(SampleInterval / portTICK_PERIOD_MS, 3 * 1024, 5, -1, internalStack){
	adc_oneshot_chan_cfg_t cfg = {
		.atten = ADC_ATTEN_DB_12,
		.bitwidth = ADC_BITWIDTH_12
	};

	hEMAFilter = newObject<EMA_ADCFilter>(this, EmaA);
	vEMAFilter = newObject<EMA_ADCFilter>(this, EmaA);

	hReader = newObject<ADCReader>(this, hPin, cfg, false, StrongObjectPtr<ADCFilter>(hEMAFilter));
	vReader = newObject<ADCReader>(this, vPin, cfg, false, StrongObjectPtr<ADCFilter>(vEMAFilter));
}

int8_t Joystick::getHorizontal() const noexcept{
	return hValue.load();
}

int8_t Joystick::getVertical() const noexcept{
	return vValue.load();
}

int8_t Joystick::mapSample(float raw, float center) noexcept{
	// Scale each side by its own span, so an off-centre stick still reaches 100% in both directions
	const float centered = raw - center;
	const float span = centered >= 0 ? static_cast<float>(RawMax) - center : center;
	int mapped = std::clamp(static_cast<int>(centered / span * 100.0f), -65, 65);
	if(mapped > -DeadZone && mapped < DeadZone){
		return 0;
	}

	mapped = static_cast<int>(100.0f * mapped / MaxMapping); // NOLINT(*-narrowing-conversions)
	return static_cast<int8_t>(mapped);
}

void Joystick::finishCalibration(){
	calibrated = true;
	for(AxisCalib* axis : { &hCalib, &vCalib }){
		const float mean = axis->sum / static_cast<float>(calibCount);
		const bool steady = axis->max - axis->min <= MaxCalibSpread;
		const bool plausible = std::fabs(mean - static_cast<float>(RawCenter)) <= MaxCenterOffset;
		axis->center = (steady && plausible) ? mean : static_cast<float>(RawCenter);
	}
	ESP_LOGI(TAG, "Centre calibrated: h=%.0f v=%.0f (nominal %d)", hCalib.center, vCalib.center, RawCenter);
}

void Joystick::tick(float deltaTime) noexcept{
	Super::tick(deltaTime);

	const float h = hReader->sample();
	const float v = vReader->sample();

	if(!calibrated){
		hCalib.add(h);
		vCalib.add(v);
		if(++calibCount >= CalibSamples){
			finishCalibration();
		}
		return; // report centre (0) until calibrated
	}

	hValue.store(-mapSample(h, hCalib.center));
	vValue.store(mapSample(v, vCalib.center));
}
