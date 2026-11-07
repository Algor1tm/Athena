#include "D3D12StagingPool.h"
#include "Athena/Math/Limits.h"
#include "Athena/Renderer/RHI/D3D12/D3D12RHI.h"
#include "Athena/Renderer/RHI/D3D12/D3D12Utils.h"


namespace Athena
{
	void D3D12StagingPool::Clear()
	{
		for (auto it = m_FreeBuffers.begin(); it != m_FreeBuffers.end();)
		{
			D3D12RHI::Get()->GetAllocator()->DestroyResource(it->second.Resource);
			it = m_FreeBuffers.erase(it);
		}

		ensuref(m_UsedBuffers.empty());
	}

	D3D12Resource D3D12StagingPool::GetBuffer(uint64 size)
	{
		for (auto it = m_FreeBuffers.begin(); it != m_FreeBuffers.end();)
		{
			if (size == it->second.Size)
			{
				return it->second.Resource;
			}
		}

		TRACY_PROFILE_SCOPE("VulkanStagingManager::Allocation");

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = size;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12Resource buffer;
		HRESULT hr = D3D12RHI::Get()->GetAllocator()->AllocateResource(&buffer, desc, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);

		if (!SUCCEEDED(hr))
		{
			Clear();

			D3D12_CHECK(D3D12RHI::Get()->GetAllocator()->AllocateResource(&buffer, desc, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD));
		}

		D3D12StagingBuffer stagingBuffer;
		stagingBuffer.Resource = buffer;
		stagingBuffer.Size = size;

		m_FreeBuffers[buffer.GetResource()] = stagingBuffer;

		return buffer;
	}

	void D3D12StagingPool::MarkAsUsed(D3D12Resource buffer)
	{
		ensuref(m_FreeBuffers.contains(buffer.GetResource()));
		ensuref(!m_UsedBuffers.contains(buffer.GetResource()));

		D3D12StagingBuffer stagingBuffer = m_FreeBuffers.at(buffer.GetResource());
		stagingBuffer.RetireFenceValue = D3D12RHI::Get()->GetFrameFence()->GetSignaledValue() + 1;

		m_FreeBuffers.erase(buffer.GetResource());
		m_UsedBuffers[buffer.GetResource()] = stagingBuffer;
	}

	void D3D12StagingPool::Update(uint64 completedFenceValue)
	{
		for (auto it = m_UsedBuffers.begin(); it != m_UsedBuffers.end();)
		{
			if (it->second.RetireFenceValue <= completedFenceValue)
			{
				it->second.LastTimeUsed = Time::GetSystemCurrentTime();

				m_FreeBuffers.emplace(it->first, std::move(it->second));
				it = m_UsedBuffers.erase(it);
			}
			else
			{
				++it;
			}
		}

		for (auto it = m_FreeBuffers.begin(); it != m_FreeBuffers.end();)
		{
			if (Time::GetSystemCurrentTime() - it->second.LastTimeUsed >= Time::Seconds(D3D12_STAGING_FREE_DELAY))
			{
				D3D12RHI::Get()->GetAllocator()->DestroyResource(it->second.Resource);
				it = m_FreeBuffers.erase(it);
			}
			else
			{
				++it;
			}
		}
	}
}
