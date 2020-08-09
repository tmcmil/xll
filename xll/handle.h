// handle.h - handles to C++ objects
#pragma once
#include <algorithm>
#include <set>
#include <memory>
#include "excel.h"

// handle data type
using HANDLEX = double;

// handle argument types for add-ins
inline const auto XLL_HANDLE= XLL_DOUBLE;
inline const auto XLL_HANDLE12 = XLL_DOUBLE12;
inline const auto XLL_HANDLEX = XLL_DOUBLEX;

namespace xll {

	template<class T>
	inline HANDLEX to_handle(T* p)
	{
		// first 16 bits of p are 0 and 2^(64 - 16) = 2^48
		// doubles exactly represent integers < 2^53
		return (HANDLEX)((uint64_t)p);
	}
	template<class T>
	inline T* to_pointer(HANDLEX h)
	{
		return (T*)((uint64_t)h);
	}

	/// <summary>
	/// Collection of handles parameterized by type.
	/// </summary>
	template<class T>
	class handle {
		/// garbage collect handles on exit
		struct pointers : public std::set<T*> {
			using std::set<T*>::begin;
			using std::set<T*>::end;
			~pointers()
			{
				std::for_each(begin(), end(), [](auto i) { delete i; });
			}
		};
		inline static pointers ps;
		inline static std::set<std::unique_ptr<T>> ps_;
		T* p;
		using ptr_ = std::unique_ptr<T, std::function<void(T*)>>;
//		inline static constexpr del = [](T*) {};
	public:
		/// <summary>
		/// Add a handle to the collection.
		/// If the calling cell has a known handle then
		/// delete the object and erase pointer.
		/// </summary>
		handle(T* p) noexcept
			: p(p)
		{
			// value in calling cell
			OPERX x = Excel<XLOPERX>(xlCoerce, Excel<XLOPERX>(xlfCaller));
			if (x.xltype == xltypeNum && x.val.num != 0) {
				// looks like a handle
				T* px = to_pointer<T>(x.val.num);
				auto pi = ps.find(px);
				if (pi != ps.end()) {
					delete* pi;
					ps.erase(pi);
				}
				auto pi_ = ps_.find(ptr_(px, [](T*) {}));
				if (pi_ != ps_.end()) {
					delete pi_->release();
					ps_.erase(pi_);
				}
			}

			ps.insert(p);
		}
		handle(HANDLEX h) noexcept
			: p(to_pointer<T>(h))
		{
			// unknown handle
			if (!ps.contains(p)) {
				p = nullptr;
			}
		}
		operator bool() const
		{
			return p != nullptr;
		}
		// return value for Excel
		HANDLEX get()
		{
			return to_handle(p);
		}
		T* ptr()
		{
			return p;
		}
		T* operator->()
		{
			return p;
		}
		const T* operator->() const
		{
			return p;
		}
	};

}
