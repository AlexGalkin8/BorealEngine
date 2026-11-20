#include <engine/render/interface/renderdevice.h>
#include <engine/render/interface/gpuresourcemanager.h>

#include <cstdio>

int main()
{
	for (int iteration = 0; iteration < 3; ++iteration)
	{
		render::IRenderDevice::CreationDesc settings{};
		auto device = render::IRenderDevice::create<render::ERenderAPI::kD3D12>(settings);
		if (!device) return 1;
		auto& manager = device->getResourceManager();

		render::IBuffer::Desc buffer_desc{};
		buffer_desc.size = 16;
		auto* first_buffer = manager.createBuffer(buffer_desc);
		render::ITexture::Desc texture_desc{};
		texture_desc.width = 1;
		texture_desc.height = 1;
		texture_desc.format = render::ETextureFormat::kRGBA8_Unorm;
		auto* first_texture = manager.createTexture(texture_desc);
		if (!first_buffer || !first_texture) return 2;
		const unsigned char pixel[] = {32, 64, 128, 255};
		first_texture->uploadPixels(pixel, sizeof(pixel));

		for (int i = 0; i < 80; ++i)
		{
			if (!manager.createBuffer(buffer_desc)) return 3;
		}
		for (int i = 0; i < 20; ++i)
		{
			if (!manager.createTexture(texture_desc)) return 4;
		}

		// Borrowed pointers must still refer to the original heap objects.
		if (first_buffer->getDesc().size != 16 || first_texture->getDesc().width != 1) return 5;
		const unsigned values[4] = {1, 2, 3, 4};
		first_buffer->upload(values, sizeof(values));
		device.reset();
		std::printf("PASS: managed pointers survive storage growth; device teardown %d\n", iteration + 1);
	}
}
