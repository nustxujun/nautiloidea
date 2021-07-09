#include "ProfileWindow.h"

ProfileWindow::ProfileWindow()
{
	mProfileWindow = ImGuiOverlay::ImGuiObject::root()->createChild<ImGuiOverlay::ImGuiWindow>("profile window");

	mProfileWindow->drawCallback = [](auto gui) {
		static bool show_inactive = false;
		static size_t selecteditem = -1;
		static std::vector<float> times(100, 0);
		static float upper = 0;
		static int interval = 4;
		static bool stop = false;
		bool resetProfile = false;
		
		ImGui::Checkbox("show inactive pass", &show_inactive);
		if (ImGui::Button("reset max"))
		{
			resetProfile = true;
			upper = 0;
		}
		static int record_count = 0;

		ImGui::SliderInt("interval", &interval, 1, 128);
		ImGui::PlotHistogram("", times.data(), (int)times.size(), 0, "", 0, upper, ImVec2{ 0,100 });
		stop = ImGui::IsItemHovered();
		
		float width = ImGui::GetWindowWidth();
		times.resize((size_t)(width * 0.125f));
		struct Node
		{
			size_t index = 0;
			const char* name;
			float cpu = 0;
			float cpumax = 0;
			float gpu = 0;
			float gpumax = 0;
			Node* parent = 0;
			std::vector<Node> children;
			bool active = false;
			bool selected = false;
			void draw()
			{

				if (!show_inactive && !active)
					return;

				bool allinvisible = true;
				for (auto& c : children)
					if (c.active)
						allinvisible = false;

				std::string format = "%s | cpu: %.3f(%.3f)";
				if (gpu >= 0.001f)
					format += " | gpu: %.3f(%.3f)";

				if (selecteditem == index)
				{
					if (!stop && record_count++ % interval == 0)
					{
						times.erase(times.begin());
						times.push_back(cpu);
						//upper = std::max(upper, cpu);
						if (cpu > upper)
							upper = (upper * 0.9f + cpu * 0.1f);
					}
				}

				if (children.empty() || (!show_inactive && allinvisible))
				{
					if (selecteditem == index)
					{
						ImGui::Selectable("", true);
						ImGui::SameLine();
					}
					ImGui::BulletText("");
					if (ImGui::IsItemClicked())
					{
						selecteditem = index;
					}
					ImGui::SameLine();

					ImGui::TextColored(active ? ImVec4{ 1,1,1,1 } : ImVec4{ 0.5,0.5,0.5,1 }, format.c_str(), name, cpu, cpumax, gpu, gpumax);
					if (ImGui::IsItemClicked())
					{
						selecteditem = index;
					}
				}
				else
				{

					bool bopen = ImGui::TreeNodeEx(name, selecteditem == index ? ImGuiTreeNodeFlags_Selected : 0, "");
					ImGui::SameLine();
					ImGui::TextColored(active ? ImVec4{ 1,1,1,1 } : ImVec4{ 0.5,0.5,0.5,1 }, format.c_str(), name, cpu, cpumax, gpu, gpumax);
					if (ImGui::IsItemClicked())
					{
						selecteditem = index;
					}
					if (!bopen)
						return;
					for (auto& c : children)
						c.draw();
					ImGui::TreePop();
				}
			}
		};

		Node root;
		root.cpu = 0;
		root.name = "all";
		root.active = true;
		Node* parent = &root;
		Node* current = 0;
		size_t curDepth = 0;
		auto cur = std::chrono::high_resolution_clock::now();
		size_t inc_index = 1;
		ProfileMgr::Singleton.visit([&](ProfileMgr::Node* node, size_t depth) {
			if (curDepth > depth)
			{
				while (curDepth > depth)
				{
					parent = parent->parent;
					curDepth--;
				}
			}
			else if (curDepth < depth)
			{
				parent = current;
				curDepth = depth;
			}

			parent->children.push_back({});
			Node& n = parent->children.back();
			auto profile = node->profile;
			if (resetProfile)
				profile->reset();
			n.index = inc_index++;
			n.name = node->name.c_str();
			n.cpu = profile->getCPUTime();
			n.cpumax = profile->getCPUMax();
			n.gpu = profile->getGPUTime();
			n.gpumax = profile->getGPUMax();
			n.parent = parent;
			n.active = (cur - node->timestamp).count() < 1000000000;
			current = &n;
			});

		root.draw();

		return true;
	};
}

ProfileWindow::~ProfileWindow()
{
	mProfileWindow->destroy();
}
