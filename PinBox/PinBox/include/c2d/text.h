#pragma once
#include "base.h"

struct C2D_TextBuf_s;
typedef struct C2D_TextBuf_s* C2D_TextBuf;

/** @defgroup Text Text drawing functions
 *  @{
 */

/// Text object.
typedef struct
{
	C2D_TextBuf buf;   ///< Buffer associated with the text.
	size_t      begin; ///< Reserved for internal use.
	size_t      end;   ///< Reserved for internal use.
	float       width; ///< Width of the text in pixels, according to 1x scale metrics.
	u32         lines; ///< Number of lines in the text, according to 1x scale metrics;
} C2D_Text;

enum
{
	C2D_AtBaseline = BIT(0), ///< Matches the Y coordinate with the baseline of the font.
	C2D_WithColor  = BIT(1), ///< Draws text with color.
};

/** @brief Creates a new text buffer.
 *  @param[in] maxGlyphs Maximum number of glyphs that can be stored in the buffer.
 *  @returns Text buffer handle (or NULL on failure).
 */
C2D_TextBuf C2D_TextBufNew(size_t maxGlyphs);

/** @brief Deletes a text buffer.
 *  @param[in] buf Text buffer handle.
 *  @remarks This also invalidates all text objects previously created with this buffer.
 */
void C2D_TextBufDelete(C2D_TextBuf buf);

/** @brief Clears all stored text in a buffer.
 *  @param[in] buf Text buffer handle.
 */
void C2D_TextBufClear(C2D_TextBuf buf);

/** @brief Retrieves the number of glyphs stored in a text buffer.
 *  @param[in] buf Text buffer handle.
 *  @returns The number of glyphs.
 */
size_t C2D_TextBufGetNumGlyphs(C2D_TextBuf buf);

/** @brief Parses and adds a single line of text to a text buffer.
 *  @param[out] text Pointer to text object to store information in.
 *  @param[in] buf Text buffer handle.
 *  @param[in] str String to parse.
 *  @param[in] lineNo Line number assigned to the text (used to calculate vertical position).
 *  @remarks Whitespace doesn't add any glyphs to the text buffer and is thus "free".
 *  @returns On success, a pointer to the character on which string processing stopped, which
 *           can be a newline ('\n'; indicating that's where the line ended), the null character
 *           ('\0'; indicating the end of the string was reached), or any other character
 *           (indicating the text buffer is full and no more glyphs can be added).
 *           On failure, NULL.
 */
const char* C2D_TextParseLine(C2D_Text* text, C2D_TextBuf buf, const char* str, u32 lineNo);

/** @brief Parses and adds arbitrary text (including newlines) to a text buffer.
 *  @param[out] text Pointer to text object to store information in.
 *  @param[in] buf Text buffer handle.
 *  @param[in] str String to parse.
 *  @remarks Whitespace doesn't add any glyphs to the text buffer and is thus "free".
 *  @returns On success, a pointer to the character on which string processing stopped, which
 *           can be the null character ('\0'; indicating the end of the string was reached),
 *           or any other character (indicating the text buffer is full and no more glyphs can be added).
 *           On failure, NULL.
 */
const char* C2D_TextParse(C2D_Text* text, C2D_TextBuf buf, const char* str);

/** @brief Optimizes a text object in order to be drawn more efficiently.
 *  @param[in] text Pointer to text object.
 */
void C2D_TextOptimize(const C2D_Text* text);

/** @brief Retrieves the total dimensions of a text object.
 *  @param[in] text Pointer to text object.
 *  @param[in] scaleX Horizontal size of the font. 1.0f corresponds to the native size of the font.
 *  @param[in] scaleY Vertical size of the font. 1.0f corresponds to the native size of the font.
 *  @param[out] outWidth (optional) Variable in which to store the width of the text.
 *  @param[out] outHeight (optional) Variable in which to store the height of the text.
 */
void C2D_TextGetDimensions(const C2D_Text* text, float scaleX, float scaleY, float* outWidth, float* outHeight);

/** @brief Draws text using the GPU.
 *  @param[in] text Pointer to text object.
 *  @param[in] flags Text drawing flags.
 *  @param[in] x Horizontal position to draw the text on.
 *  @param[in] y Vertical position to draw the text on. If C2D_AtBaseline is not specified (default), this
 *               is the top left corner of the block of text; otherwise this is the position of the baseline
 *               of the first line of text.
 *  @param[in] z Depth value of the text. If unsure, pass 0.0f.
 *  @param[in] scaleX Horizontal size of the font. 1.0f corresponds to the native size of the font.
 *  @param[in] scaleY Vertical size of the font. 1.0f corresponds to the native size of the font.
 *  @remarks The default 3DS system font has a glyph height of 30px, and the baseline is at 25px.
 */
void C2D_DrawText(const C2D_Text* text, u32 flags, float x, float y, float z, float scaleX, float scaleY, ...);

/** @} */
