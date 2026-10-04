#pragma once
#include <Windows.h>

namespace ssa::D3DXRuntime
{
    // minimal stand-in for ID3DXBuffer (same vtable layout)
    struct IBlob : public IUnknown
    {
        virtual LPVOID STDMETHODCALLTYPE GetBufferPointer() = 0;
        virtual DWORD STDMETHODCALLTYPE GetBufferSize() = 0;
    };

    // Fn is the function pointer type, e.g. GetFunction<D3DXAssembleShader_t>("D3DXAssembleShader"). nullptr if missing.
    template <class Fn>
    inline Fn GetFunction(const char* name)
    {
        HMODULE h = GetModuleHandleA("d3dx9_42.dll");
        if (!h) h = LoadLibraryA("d3dx9_42.dll");
        return h ? reinterpret_cast<Fn>(GetProcAddress(h, name)) : nullptr;
    }

    using D3DXDisassembleShader_t = HRESULT(WINAPI*)(const DWORD* pShader, BOOL enableColorCode, LPCSTR pComments, IBlob** ppDisassembly);

    // owns one reference to a blob, releases it on scope exit
    class BlobPtr
    {
    public:
        BlobPtr() = default;
        BlobPtr(const BlobPtr&) = delete;
        BlobPtr& operator=(const BlobPtr&) = delete;
        ~BlobPtr() { if (m_blob) m_blob->Release(); }

        IBlob** Receive() { return &m_blob; } // pass to the D3DX function that fills it in
        explicit operator bool() const { return m_blob != nullptr; }
        const void* Data() const { return m_blob ? m_blob->GetBufferPointer() : nullptr; }
        size_t Size() const { return m_blob ? m_blob->GetBufferSize() : 0; }
        const char* Text() const { return static_cast<const char*>(Data()); } // for the text blobs D3DX returns

    private:
        IBlob* m_blob = nullptr;
    };
} // namespace ssa::D3DXRuntime
