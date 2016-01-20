#ifndef ARIS_MOTION_H
#define ARIS_MOTION_H

#include <functional>
#include <thread>
#include <atomic>

#include "aris_ethercat.h"


namespace Aris
{
	namespace Control
	{	
		class EthercatMotion :public EthercatSlave
		{
		public:
			enum Cmd
			{
				IDLE = 0,
				ENABLE,
				DISABLE,
				HOME,
				RUN
			};
			enum Mode
			{
				POSITION = 0x0008,
				VELOCITY = 0x0009,
				CURRENT = 0x0010,
			};
			struct Data
			{
				std::int32_t targetPos{ 0 }, feedbackPos{ 0 };
				std::int32_t targetVel{ 0 }, feedbackVel{ 0 };
				std::int16_t targetCur{ 0 }, feedbackCur{ 0 };
				std::uint8_t cmd{ IDLE };
				std::uint8_t mode{ POSITION };
				mutable std::int16_t ret{ 0 };
			};

			virtual ~EthercatMotion();
			EthercatMotion(const Aris::Core::XmlElement *);
			
			bool HasFault();
			void ReadFeedback(Data &data);
			void DoCommand(const Data &data);
			std::int32_t MaxSpeed() { return max_speed; };
			std::int32_t ID() { return model_id; };
			std::int32_t InputRatio() { return input2count; };

		protected:
			virtual void Init() override;

		private:
			std::int32_t input2count;
			std::int32_t max_speed;
			std::int32_t home_pos;
			std::int32_t model_id;
			

			class Imp;
			std::unique_ptr<Imp> pImp;

			friend class EthercatController;
		};
		class EthercatForceSensor :public EthercatSlave
		{
		public:
			struct Data
			{
				union
				{
					struct { double Fx, Fy, Fz, Mx, My, Mz; };
					double fce[6];
				};
				
			};

			EthercatForceSensor(const Aris::Core::XmlElement *ele): EthercatSlave(ele){};
			void ReadData(Data &data);
		};

		class EthercatController :public EthercatMaster
		{
		public:
			struct Data
			{
				const std::vector<EthercatMotion::Data> *pLastMotionData;
				std::vector<EthercatMotion::Data> *pMotionData;
				std::vector<EthercatForceSensor::Data> *pForceSensorData;
				const Aris::Core::MsgRT *pMsgRecv;
				Aris::Core::MsgRT *pMsgSend;
			};

			virtual ~EthercatController(){};
			virtual void LoadXml(const Aris::Core::XmlElement *) override;
			virtual void Start();
			virtual void Stop();
			void SetControlStrategy(std::function<int(Data&)>);
			
			EthercatMotion * MotionAt(int i) { return pMotions.at(i); };
			EthercatForceSensor* ForceSensorAt(int i) { return pForceSensors.at(i); };
			Pipe<Aris::Core::Msg>& MsgPipe(){return msgPipe;};
			

		protected:
			EthercatController() :EthercatMaster(),msgPipe(0, true) {};
			virtual void ControlStrategy() override;

		private:
			std::function<int(Data&)> strategy;
			Pipe<Aris::Core::Msg> msgPipe;
			std::atomic_bool isStoping;

			std::vector<EthercatMotion *> pMotions;
			std::vector<EthercatMotion::Data> motionData, lastMotionData;

			std::vector<EthercatForceSensor *> pForceSensors;
			std::vector<EthercatForceSensor::Data> forceSensorData;

			std::unique_ptr<Pipe<std::vector<EthercatMotion::Data> > > pMotDataPipe;
			std::thread motionDataThread;

			friend class EthercatMaster;
		};
	}
}

#endif