#include "stdafx.h"
#include "CppUnitTest.h"


#include "../../ztTrace/TracePoint.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace zt;

namespace ztTrackTests
{
	TEST_CLASS(UnitTest1)
	{
	public:

		OptimizationParameters pars{ 1.0, 2.0, 400.0 , 2};
		std::vector<std::shared_ptr<TracePointAuto>> seg_ptrs(std::vector<TracePointAuto>& seg){
			std::vector<std::shared_ptr<TracePointAuto>> r;
			for (TracePointAuto&p : seg) { r.push_back(std::make_shared<TracePointAuto>(p)); }
			return r;
		}
		std::vector<std::shared_ptr<TracePointAuto>> make_segment(int count){
			std::vector<std::shared_ptr<TracePointAuto>> r{};
			for (int i = 0; i < count; i++) r.push_back(std::make_shared<TracePointAuto>());
			return r;
		}

		TEST_METHOD(tracepoint_auto_no_matches)
		{
			TracePointKeyFrame start{ Patch(0, 0), Descriptor(1, 1.0) };
			TracePointKeyFrame end{ Patch(20, 20), Descriptor(1, 3.0) };
			auto segment = make_segment(1);
			Assert::IsNull(segment[0]->location(), L"before");
			TracePointAuto::optimize(segment, start, end, pars);
			Assert::IsNull(segment[0]->location(), L"after");
		}

		TEST_METHOD(simplest_linear_trace)
		{
			TracePointKeyFrame start{ Patch(0, 0), Descriptor(1, 1.0) };
			TracePointKeyFrame end{ Patch(20, 20), Descriptor(1, 3.0) };
			auto segment = make_segment(1);
			segment[0]->add_match(Patch(10, 11), Descriptor(1, 1.0), 0, 0.0);

			Assert::IsNull(segment[0]->location(), L"before");
			TracePointAuto::optimize(segment, start, end, pars);
			Assert::IsNotNull(segment[0]->location(), L"after");
			Assert::AreEqual(10, segment[0]->location()->x());
			Assert::AreEqual(11, segment[0]->location()->y());
		}

		TEST_METHOD(prefer_occlude_distance)
		{
			TracePointKeyFrame start{ Patch(0, 0), Descriptor(1, 1.0) };
			TracePointKeyFrame end{ Patch(20, 20), Descriptor(1, 3.0) };
			auto segment = make_segment(1);
			segment[0]->add_match(Patch(20, 0), Descriptor(1, 1.0), 0, 0.0);
			Assert::IsNull(segment[0]->location(), L"before");
			TracePointAuto::optimize(segment, start, end, pars);
			Assert::IsNull(segment[0]->location(), L"occlude after");
		}

		TEST_METHOD(prefer_occlude_appearance)
		{
			TracePointKeyFrame start{ Patch(0, 0), Descriptor(1, 1.0) };
			TracePointKeyFrame end{ Patch(20, 20), Descriptor(1, 3.0) };
			auto segment = make_segment(1);
			segment[0]->add_match(Patch(10, 11), Descriptor(1, -9.0), 0, 100.0);
			Assert::IsNull(segment[0]->location(), L"before");
			TracePointAuto::optimize(segment, start, end, pars);
			Assert::IsNull(segment[0]->location(), L"occlude after");
		}

		TEST_METHOD(open_start_prefer_closest)
		{
			TracePointOccluded start;
			TracePointKeyFrame end{ Patch(20, 20), Descriptor(1, 3.0) };
			auto segment = make_segment(1);
			segment[0]->add_match(Patch(10, 11), Descriptor(1, 1.0), 0, 0.0);
			segment[0]->add_match(Patch(12, 10), Descriptor(1, 1.0), 0, 0.0);
			Assert::IsNull(segment[0]->location(), L"before");
			TracePointAuto::optimize(segment, start, end, pars);
			Assert::IsNotNull(segment[0]->location(), L"after");
			Assert::AreEqual(12, segment[0]->location()->x());
			Assert::AreEqual(10, segment[0]->location()->y());
		}


		TEST_METHOD(open_start_decline_different_and_far)
		{
			TracePointOccluded start;
			TracePointKeyFrame end{ Patch(20, 20), Descriptor(1, 3.0) };
			auto segment = make_segment(1);
			segment[0]->add_match(Patch(10, 0), Descriptor(1, 1.0), 0, 0.0);
			segment[0]->add_match(Patch(12, 10), Descriptor(1, -9.0), 0, 100.0);
			Assert::IsNull(segment[0]->location(), L"before");
			TracePointAuto::optimize(segment, start, end, pars);
			Assert::IsNull(segment[0]->location(), L"occlude after");
		}
		TEST_METHOD(open_end_prefer_closest)
		{
			TracePointOccluded end;
			TracePointKeyFrame start{ Patch(20, 20), Descriptor(1, 3.0) };
			auto segment = make_segment(1);
			segment[0]->add_match(Patch(10, 11), Descriptor(1, 1.0), 0, 0.0);
			segment[0]->add_match(Patch(12, 10), Descriptor(1, 1.0), 0, 0.0);
			Assert::IsNull(segment[0]->location(), L"before");
			TracePointAuto::optimize(segment, start, end, pars);
			Assert::IsNotNull(segment[0]->location(), L"after");
			Assert::AreEqual(12, segment[0]->location()->x());
			Assert::AreEqual(10, segment[0]->location()->y());
		}


		TEST_METHOD(open_end_decline_different_and_far)
		{
			TracePointOccluded end;
			TracePointKeyFrame start{ Patch(20, 20), Descriptor(1, 3.0) };
			auto segment = make_segment(1);
			segment[0]->add_match(Patch(10, 0), Descriptor(1, 1.0), 0, 0.0);
			segment[0]->add_match(Patch(12, 10), Descriptor(1, -9.0), 0, 100.0);
			Assert::IsNull(segment[0]->location(), L"before");
			TracePointAuto::optimize(segment, start, end, pars);
			Assert::IsNull(segment[0]->location(), L"occlude after");
		}

		TEST_METHOD(both_end_are_open)
		{
			TracePointOccluded start;
			TracePointOccluded end;
			auto segment = make_segment(1);
			segment[0]->add_match(Patch(10, 11), Descriptor(1, 1.0), 0, 0.0);

			Assert::IsNull(segment[0]->location(), L"before");
			TracePointAuto::optimize(segment, start, end, pars);
			Assert::IsNotNull(segment[0]->location(), L"after");
			Assert::AreEqual(10, segment[0]->location()->x());
			Assert::AreEqual(11, segment[0]->location()->y());
		}

		TEST_METHOD(long_prefer_shortest)
		{
			TracePointKeyFrame start{ Patch(0, 0), Descriptor(1, 1.0) };
			TracePointKeyFrame end{ Patch(30, 30), Descriptor(1, 3.0) };
			auto segment = make_segment(2);
			segment[0]->add_match(Patch(10, 12), Descriptor(1, 1.0), 0, 0.0);
			segment[0]->add_match(Patch(10, 11), Descriptor(1, 1.0), 0, 0.0);
			segment[1]->add_match(Patch(20, 22), Descriptor(1, 1.0), 0, 0.0);
			segment[1]->add_match(Patch(20, 21), Descriptor(1, 1.0), 0, 0.0);

			Assert::IsNull(segment[0]->location(), L"before[0]");
			Assert::IsNull(segment[1]->location(), L"before[1]");
			TracePointAuto::optimize(segment, start, end, pars);
			Assert::IsNotNull(segment[0]->location(), L"after[0]");
			Assert::AreEqual(10, segment[0]->location()->x());
			Assert::AreEqual(11, segment[0]->location()->y());
			Assert::IsNotNull(segment[1]->location(), L"after[1]");
			Assert::AreEqual(20, segment[1]->location()->x());
			Assert::AreEqual(21, segment[1]->location()->y());
		}

		TEST_METHOD(long_w_occluded_prefer_shortest)
		{
			TracePointKeyFrame start{ Patch(0, 0), Descriptor(1, 1.0) };
			TracePointKeyFrame end{ Patch(30, 30), Descriptor(1, 3.0) };
			auto segment = make_segment(3);
			segment[0]->add_match(Patch(10, 12), Descriptor(1, 1.0), 0, 0.0);
			segment[0]->add_match(Patch(10, 11), Descriptor(1, 1.0), 0, 0.0);
			segment[1]->add_match(Patch(20, 22), Descriptor(1, 1.0), 0, 0.0);
			segment[1]->add_match(Patch(20, 21), Descriptor(1, 1.0), 0, 0.0);

			Assert::IsNull(segment[0]->location(), L"before[0]");
			Assert::IsNull(segment[1]->location(), L"before[1]");
			Assert::IsNull(segment[2]->location(), L"before[2]");
			TracePointAuto::optimize(segment, start, end, pars);
			Assert::IsNotNull(segment[0]->location(), L"after[0]");
			Assert::AreEqual(10, segment[0]->location()->x());
			Assert::AreEqual(11, segment[0]->location()->y());
			Assert::IsNotNull(segment[1]->location(), L"after[1]");
			Assert::AreEqual(20, segment[1]->location()->x());
			Assert::AreEqual(21, segment[1]->location()->y());
			Assert::IsNull(segment[2]->location(), L"after[2]");
		}

		TEST_METHOD(long_w_occluded2_prefer_shortest)
		{
			TracePointKeyFrame start{ Patch(0, 0), Descriptor(1, 1.0) };
			TracePointKeyFrame end{ Patch(30, 30), Descriptor(1, 3.0) };
			auto segment = make_segment(3);
			segment[1]->add_match(Patch(10, 12), Descriptor(1, 1.0), 0, 0.0);
			segment[1]->add_match(Patch(14, 13), Descriptor(1, 1.0), 0, 0.0);
			segment[2]->add_match(Patch(20, 22), Descriptor(1, 1.0), 0, 0.0);
			segment[2]->add_match(Patch(22, 20), Descriptor(1, 1.0), 0, 0.0);

			Assert::IsNull(segment[0]->location(), L"before[0]");
			Assert::IsNull(segment[1]->location(), L"before[0]");
			Assert::IsNull(segment[2]->location(), L"before[1]");
			TracePointAuto::optimize(segment, start, end, pars);
			Assert::IsNull(segment[0]->location(), L"after[0]");
			Assert::IsNotNull(segment[1]->location(), L"after[0]");
			Assert::AreEqual(14, segment[1]->location()->x());
			Assert::AreEqual(13, segment[1]->location()->y());
			Assert::IsNotNull(segment[2]->location(), L"after[1]");
			Assert::AreEqual(22, segment[2]->location()->x());
			Assert::AreEqual(20, segment[2]->location()->y());
		}

		TEST_METHOD(long_prefer_similar)
		{
			TracePointKeyFrame start{ Patch(0, 0), Descriptor(1, 1.0) };
			TracePointKeyFrame end{ Patch(30, 30), Descriptor(1, 3.0) };
			auto segment = make_segment(2);
			segment[0]->add_match(Patch(10, 12), Descriptor(1, 1.0), 0, 0.0);
			segment[0]->add_match(Patch(10, 11), Descriptor(1, -1.0), 0, 4.0);
			segment[1]->add_match(Patch(20, 22), Descriptor(1, 1.0), 0, 0.0);
			segment[1]->add_match(Patch(20, 21), Descriptor(1, -1.0), 0, 4.0);

			Assert::IsNull(segment[0]->location(), L"before[0]");
			Assert::IsNull(segment[1]->location(), L"before[1]");
			TracePointAuto::optimize(segment, start, end, pars);
			Assert::IsNotNull(segment[0]->location(), L"after[0]");
			Assert::AreEqual(10, segment[0]->location()->x());
			Assert::AreEqual(12, segment[0]->location()->y());
			Assert::IsNotNull(segment[1]->location(), L"after[1]");
			Assert::AreEqual(20, segment[1]->location()->x());
			Assert::AreEqual(22, segment[1]->location()->y());
		}

		TEST_METHOD(long_occlude_distance)
		{
			TracePointKeyFrame start{ Patch(0, 0), Descriptor(1, 1.0) };
			TracePointKeyFrame end{ Patch(30, 30), Descriptor(1, 3.0) };
			auto segment = make_segment(2);
			segment[0]->add_match(Patch(10, 10), Descriptor(1, 1.0), 0, 0.0);
			segment[1]->add_match(Patch(30, 10), Descriptor(1, 1.0), 0, 0.0);

			Assert::IsNull(segment[0]->location(), L"before[0]");
			Assert::IsNull(segment[1]->location(), L"before[1]");
			TracePointAuto::optimize(segment, start, end, pars);
			Assert::IsNotNull(segment[0]->location(), L"after[0]");
			Assert::AreEqual(10, segment[0]->location()->x());
			Assert::AreEqual(10, segment[0]->location()->y());
			Assert::IsNull(segment[1]->location(), L"after[1]");
		}

		TEST_METHOD(long_occlude2_distance)
		{
			TracePointKeyFrame start{ Patch(0, 0), Descriptor(1, 1.0) };
			TracePointKeyFrame end{ Patch(30, 30), Descriptor(1, 3.0) };
			auto segment = make_segment(2);
			segment[0]->add_match(Patch(30, 0), Descriptor(1, 1.0), 0, 0.0);
			segment[1]->add_match(Patch(30, 10), Descriptor(1, 1.0), 0, 0.0);

			Assert::IsNull(segment[0]->location(), L"before[0]");
			Assert::IsNull(segment[1]->location(), L"before[1]");
			TracePointAuto::optimize(segment, start, end, pars);
			Assert::IsNull(segment[0]->location(), L"after[0]");
			Assert::IsNull(segment[1]->location(), L"after[1]");
		}

		TEST_METHOD(long_occlude_appearance)
		{
			TracePointKeyFrame start{ Patch(0, 0), Descriptor(1, 1.0) };
			TracePointKeyFrame end{ Patch(30, 30), Descriptor(1, 3.0) };
			auto segment = make_segment(2);
			segment[0]->add_match(Patch(10, 10), Descriptor(1, 1.0), 0, 0.0);
			segment[1]->add_match(Patch(20, 20), Descriptor(1, -9.0), 0, 100.0);

			Assert::IsNull(segment[0]->location(), L"before[0]");
			Assert::IsNull(segment[1]->location(), L"before[1]");
			TracePointAuto::optimize(segment, start, end, pars);
			Assert::IsNotNull(segment[0]->location(), L"after[0]");
			Assert::AreEqual(10, segment[0]->location()->x());
			Assert::AreEqual(10, segment[0]->location()->y());
			Assert::IsNull(segment[1]->location(), L"after[1]");
		}

		TEST_METHOD(long_occlude_appearance2)
		{
			TracePointKeyFrame start{ Patch(0, 0), Descriptor(1, 1.0) };
			TracePointKeyFrame end{ Patch(30, 30), Descriptor(1, 3.0) };
			auto segment = make_segment(2);
			segment[0]->add_match(Patch(10, 10), Descriptor(1, -9.0), 0, 100.0);
			segment[1]->add_match(Patch(20, 20), Descriptor(1, 1.0), 0, 0.0);

			Assert::IsNull(segment[0]->location(), L"before[0]");
			Assert::IsNull(segment[1]->location(), L"before[1]");
			TracePointAuto::optimize(segment, start, end, pars);
			Assert::IsNull(segment[0]->location(), L"after[0]");
			Assert::IsNotNull(segment[1]->location(), L"after[1]");
			Assert::AreEqual(20, segment[1]->location()->x());
			Assert::AreEqual(20, segment[1]->location()->y());
		}
	};
}