	static __declspec(naked) void TexturePaletteUnderscorePatch()
	{
		__asm
		{
			mov		esi, eax
			sub		esi, ebx
			add		esp, 8

			test	eax, eax
			jnz		notNull

			push	004A27A1h
			ret

		notNull:
			push	004A26FEh
			ret
		}
	}
