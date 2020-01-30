/****************************************************************************
 *
 *   Copyright (c) 2020 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file zero_order_thrust_ekf.hpp
 * @brief Implementation of a single-state hover thrust estimator
 *
 * state: hover thrust (Th)
 * Vertical acceleration is used as a measurement and the current
 * thrust (T[k]) is used in the measurement model.
 *
 * The sate is noise driven: Transition matrix A = 1
 * x[k+1] = Ax[k] + v with v ~ N(0, Q)
 * y[k] = h(u, x) + w with w ~ N(0, R)
 *
 * Where the measurement model and corresponding partial derivative (w.r.t. Th) are:
 * h(u, x)[k] = g * T[k] / Th[k] - g
 * H[k] = -g * T[k] / Th[k]**2
 *
 * @author Mathieu Bresciani 	<brescianimathieu@gmail.com>
 */

#pragma once

#include <matrix/matrix/math.hpp>
#include <ecl/geo/geo.h>

class ZeroOrderHoverThrustEkf
{
public:
	struct status {
		float hover_thrust;
		float hover_thrust_var;
		float innov;
		float innov_var;
		float innov_test_ratio;
		float accel_noise_var;
	};

	ZeroOrderHoverThrustEkf() = default;
	~ZeroOrderHoverThrustEkf() = default;

	void resetAccelNoise() { _R = 5.f; };

	void predict(float _dt);
	status fuseAccZ(float acc_z, float thrust);

	void setProcessNoiseStdDev(float process_noise) { _Q = process_noise * process_noise; }
	void setMeasurementNoiseStdDev(float measurement_noise) { _R = measurement_noise * measurement_noise; }
	void setHoverThrustStdDev(float hover_thrust_noise) { _P = hover_thrust_noise * hover_thrust_noise; }
	void setAccelInnovGate(float gate_size) { _gate_size = gate_size; }

	float getHoverThrustEstimate() const { return _hover_thr; }

private:
	float _hover_thr{0.5f};

	float _gate_size{3.f};
	float _P{0.01f}; // Initial hover thrust uncertainty variance (thrust^2)
	float _Q{0.25e-6f}; // Hover thrust process noise (thrust^2/s^2)
	float _R{5.f}; // Acceleration variance (m^2/s^3)
	float _dt{0.02f};

	float computeH(float thrust) const;
	float computeInnovVar(float H) const;
	float computePredictedAccZ(float thrust) const;
	float computeInnov(float acc_z, float thrust) const;
	float computeKalmanGain(float H, float innov_var) const;

	/*
	 * Compute the ratio between the Normalized Innovation Squared (NIS)
	 * and its maximum gate size. Use isTestRatioPassing to know if the
	 * measurement should be fused or not.
	 */
	float computeInnovTestRatio(float innov, float innov_var) const;
	bool isTestRatioPassing(float innov_test_ratio) const;

	void updateState(float K, float innov);
	void updateStateCovariance(float K, float H);
	void updateMeasurementNoise(float residual, float H);

	status packStatus(float innov, float innov_var, float innov_test_ratio) const;

	static constexpr float noise_learning_time_constant = 0.5f; // in seconds
};
