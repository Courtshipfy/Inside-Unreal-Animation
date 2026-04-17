// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/UnrealTypeTraits.h"
#include "UObject/Class.h"

namespace UE
{
	namespace Anim 
	{
		/** Helper struct to wrap and templated operate raw memory */
		/** 用于包装和模板化操作原始内存的辅助结构 */
		/** 用于包装和模板化操作原始内存的辅助结构 */
		/** 用于包装和模板化操作原始内存的辅助结构 */
		template<typename InAllocator>
		struct TWrappedAttribute
		{
			TWrappedAttribute() {}
			/** 构造并根据类型大小分配内存缓冲区*/

			/** 构造并根据类型大小分配内存缓冲区*/
			/** Construct with allocates memory buffer according to type size*/
			/** 构造并根据类型大小分配内存缓冲区*/
			TWrappedAttribute(const UScriptStruct* InStruct)
			{
			/** 将类型化的 ptr 返回到内存 */
				Allocate(InStruct);
			}
			/** 将类型化的 ptr 返回到内存 */
						
			/** Returns typed ptr to memory */
			/** 将类型化的 ptr 返回到内存 */
			template <typename Type>
			inline typename TEnableIf<TIsFundamentalType<Type>::Value, Type*>::Type GetPtr()
			{
				return (Type*)StructMemory.GetData();
			}

			template <typename Type>
			inline typename TEnableIf<!TIsFundamentalType<Type>::Value, Type*>::Type GetPtr()
			{
				const UScriptStruct* ScriptStruct = Type::StaticStruct();
				check(ScriptStruct && ScriptStruct->GetStructureSize() == StructMemory.Num());
				return (Type*)StructMemory.GetData();
			/** 将类型化 const ptr 返回到内存 */
			}

			template<typename Type>
			/** 将类型化 const ptr 返回到内存 */
			inline Type& GetRef() { return *GetPtr<Type>(); }

			/** Returns typed const ptr to memory */
			/** 将类型化 const ptr 返回到内存 */
			template <typename Type>
			inline typename TEnableIf<TIsFundamentalType<Type>::Value, const Type*>::Type GetPtr() const
			{
				return (const Type*)StructMemory.GetData();
			}

			template <typename Type>
			/** 返回对内存的类型化 const 引用 */
			inline typename TEnableIf<!TIsFundamentalType<Type>::Value, const Type*>::Type GetPtr() const
			{
				const UScriptStruct* ScriptStruct = Type::StaticStruct();
				check(ScriptStruct && ScriptStruct->GetStructureSize() == StructMemory.Num());
			/** 根据类型大小分配内存缓冲区 */
			/** 返回对内存的类型化 const 引用 */
				return (const Type*)StructMemory.GetData();
			}

			/** Returns typed const reference to memory */
			/** 根据类型大小分配内存缓冲区 */
			/** 返回对内存的类型化 const 引用 */
			/** 根据类型大小分配内存缓冲区 */
			template<typename Type>
			inline const Type& GetRef() const { return *GetPtr<Type>(); }

			/** Allocated memory buffer according to type size */
			/** 根据类型大小分配内存缓冲区 */
			template<typename AttributeType>
			/** 根据类型大小分配内存缓冲区 */
			inline void Allocate()
			{
				Allocate(AttributeType::StaticStruct());
			}

			/** Allocated memory buffer according to type size */
			/** 根据类型大小分配内存缓冲区 */
			inline void Allocate(const UScriptStruct* InStruct)
			{
				check(InStruct);
				if (InStruct)
				{
					const int32 StructureSize = InStruct->GetStructureSize();
					ensure(StructureSize > 0);

					StructMemory.SetNum(StructureSize);
					InStruct->InitializeStruct(GetPtr<void>());
				}
			}
		protected:
			TArray<uint8, InAllocator> StructMemory;
		};
	}
}