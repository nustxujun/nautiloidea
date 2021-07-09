#pragma once 

#include "SimpleMath.h"
#include "Mouse.h"
#include "Keyboard.h"

class CameraController
{
public:
	using Vector2 = DirectX::SimpleMath::Vector2;
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;

	struct State
	{
		DirectX::Mouse::State ms;
		DirectX::Keyboard::State ks;

		Vector2 offset = Vector3::Zero;
		Vector3 pos = Vector3::Zero;
		Vector3 dir = Vector3::Zero;
	};

	static State state;

	static DirectX::SimpleMath::Matrix update(float dtime, DirectX::Keyboard& k, DirectX::Mouse& m)
	{
		auto ms = m.GetState();
		auto ks = k.GetState();

		state.offset = { float(state.ms.x - ms.x), float(state.ms.y - ms.y)};
		state.ms = ms;
		state.ks = ks;

		return updateFreeview(dtime);
	}


private:

	using Quat = DirectX::SimpleMath::Quaternion;
	static DirectX::SimpleMath::Matrix updateFreeview(float dtime)
	{
		static Quat orien = Quat::Identity;
		const float pi = 3.14159265358;

		Vector3 dir = Vector3::Transform(Vector3::UnitZ, orien);
		if (state.ms.leftButton)
		{
			Quat rot = Quat::CreateFromAxisAngle(dir.Cross(Vector3::UnitY), state.offset.y * dtime) *
				Quat::CreateFromAxisAngle(Vector3::UnitY, state.offset.x * dtime );
		
			orien = orien * rot;
		}

		dir = Vector3::Transform(Vector3::UnitZ, orien);
		
		Vector3 offset = Vector3::Zero;
		if (state.ks.W)
		{
			offset = dir * dtime;
		}
		else if (state.ks.S)
		{
			offset = -dir* dtime;
		}

		state.dir = dir;
		state.pos += offset * 100;



		Matrix view = Matrix::CreateLookAt(state.pos , state.pos + dir, Vector3::UnitY);
		return view ;
	}



};

