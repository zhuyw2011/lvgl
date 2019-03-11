/**
 * @file lv_font.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_font.h"
#include "lv_log.h"
#include "lv_math.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static int32_t lv_font_codeCompare (const void* pRef, const void* pElement);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Initialize the fonts
 */
void lv_font_init(void)
{
    lv_font_builtin_init();
}

/**
 * Add a font to an other to extend the character set.
 * @param child the font to add
 * @param parent this font will be extended. Using it later will contain the characters from `child`
 */
void lv_font_add(lv_font_t * child, lv_font_t * parent)
{
    if(parent == NULL) return;

    while(parent->next_page != NULL) {
        parent = parent->next_page; /*Got to the last page and add the new font there*/
    }

    parent->next_page = child;

}

/**
 * Remove a font from a character set.
 * @param child the font to remove
 * @param parent remove `child` from here
 */
void lv_font_remove(lv_font_t * child, lv_font_t * parent)
{
    if(parent == NULL) return;
    if(child == NULL) return;

    while(parent->next_page != child) {
        parent = parent->next_page; /*Got to the last page and add the new font there*/
    }

    parent->next_page = child->next_page;
}


/**
 * Tells if font which contains `letter` is monospace or not
 * @param font_p point to font
 * @param letter an UNICODE character code
 * @return true: the letter is monospace; false not monospace
 */
bool lv_font_is_monospace(const lv_font_t * font_p, uint32_t letter)
{
    const lv_font_t * font_i = font_p;
    int16_t w;
    while(font_i != NULL) {
        w = font_i->get_width(font_i, letter);
        if(w >= 0) {
            /*Glyph found*/
            if(font_i->monospace) return true;
            return false;
        }

        font_i = font_i->next_page;
    }

    return 0;
}

/**
 * Return with the bitmap of a font.
 * @param font_p pointer to a font
 * @param letter an UNICODE character code
 * @return  pointer to the bitmap of the letter
 */
const uint8_t * lv_font_get_bitmap(const lv_font_t * font_p, uint32_t letter)
{
    const lv_font_t * font_i = font_p;
    while(font_i != NULL) {
        const uint8_t * bitmap = font_i->get_bitmap(font_i, letter);
        if(bitmap) return bitmap;

        font_i = font_i->next_page;
    }

    return NULL;
}

/**
 * Get the width of a letter in a font. If `monospace` is set then return with it.
 * @param font_p pointer to a font
 * @param letter an UNICODE character code
 * @return the width of a letter
 */
uint8_t lv_font_get_width(const lv_font_t * font_p, uint32_t letter)
{
    const lv_font_t * font_i = font_p;
    int16_t w;
    while(font_i != NULL) {
        w = font_i->get_width(font_i, letter);
        if(w >= 0) {
            /*Glyph found*/
            uint8_t m = font_i->monospace;
            if(m) w = m;
            return w;
        }

        font_i = font_i->next_page;
    }

    return 0;

}

/**
 * Get the width of the letter without overwriting it with the `monospace` attribute
 * @param font_p pointer to a font
 * @param letter an UNICODE character code
 * @return the width of a letter
 */
uint8_t lv_font_get_real_width(const lv_font_t * font_p, uint32_t letter)
{
    const lv_font_t * font_i = font_p;
    int16_t w;
    while(font_i != NULL) {
        w = font_i->get_width(font_i, letter);
        if(w >= 0) return w;

        font_i = font_i->next_page;
    }

    return 0;
}

/**
 * Get the bit-per-pixel of font
 * @param font pointer to font
 * @param letter a letter from font (font extensions can have different bpp)
 * @return bpp of the font (or font extension)
 */
uint8_t lv_font_get_bpp(const lv_font_t * font, uint32_t letter)
{
    const lv_font_t * font_i = font;
    while(font_i != NULL) {
        if(letter >= font_i->unicode_first && letter <= font_i->unicode_last) {
            return font_i->bpp;
        }
        font_i = font_i->next_page;
    }

    return 0;

}

/**
 * Generic bitmap get function used in 'font->get_bitmap' when the font contains all characters in the range
 * @param font pointer to font
 * @param unicode_letter an unicode letter which bitmap should be get
 * @return pointer to the bitmap or NULL if not found
 */
const uint8_t * lv_font_get_bitmap_continuous(const lv_font_t * font, uint32_t unicode_letter)
{
    /*Check the range*/
    if(unicode_letter < font->unicode_first || unicode_letter > font->unicode_last) return NULL;

    uint32_t index = (unicode_letter - font->unicode_first);
    return &font->glyph_bitmap[font->glyph_dsc[index].glyph_index];
}

/**
 * Generic bitmap get function used in 'font->get_bitmap' when the font NOT contains all characters in the range (sparse)
 * @param font pointer to font
 * @param unicode_letter an unicode letter which bitmap should be get
 * @return pointer to the bitmap or NULL if not found
 */
const uint8_t * lv_font_get_bitmap_sparse(const lv_font_t * font, uint32_t unicode_letter)
{
    /*Check the range*/
    if(unicode_letter < font->unicode_first || unicode_letter > font->unicode_last) return NULL;

    uint32_t* pUnicode;

    pUnicode = lv_bsearch(&unicode_letter,
                          (uint32_t*) font->unicode_list,
                          font->glyph_cnt,
                          sizeof(uint32_t),
                          lv_font_codeCompare);

    if (pUnicode != NULL) {
        uint32_t idx = (uint32_t) (pUnicode - font->unicode_list);
        return &font->glyph_bitmap[font->glyph_dsc[idx].glyph_index];
    }

    return NULL;
}

/**
 * Generic glyph width get function used in 'font->get_width' when the font contains all characters in the range
 * @param font pointer to font
 * @param unicode_letter an unicode letter which width should be get
 * @return width of the gylph or -1 if not found
 */
int16_t lv_font_get_width_continuous(const lv_font_t * font, uint32_t unicode_letter)
{
    /*Check the range*/
    if(unicode_letter < font->unicode_first || unicode_letter > font->unicode_last) {
        return -1;
    }

    uint32_t index = (unicode_letter - font->unicode_first);
    return font->glyph_dsc[index].w_px;
}

/**
 * Generic glyph width get function used in 'font->get_bitmap' when the font NOT contains all characters in the range (sparse)
 * @param font pointer to font
 * @param unicode_letter an unicode letter which width should be get
 * @return width of the glyph or -1 if not found
 */
int16_t lv_font_get_width_sparse(const lv_font_t * font, uint32_t unicode_letter)
{
    /*Check the range*/
    if(unicode_letter < font->unicode_first || unicode_letter > font->unicode_last) return -1;

    uint32_t* pUnicode;

    pUnicode = lv_bsearch(&unicode_letter,
                          (uint32_t*) font->unicode_list,
                          font->glyph_cnt,
                          sizeof(uint32_t),
                          lv_font_codeCompare);

    if (pUnicode != NULL) {
        uint32_t idx = (uint32_t) (pUnicode - font->unicode_list);
        return font->glyph_dsc[idx].w_px;
    }

    return -1;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/** Code Comparator.
 *
 *  Compares the value of both input arguments.
 *
 *  @param[in]  pRef        Pointer to the reference.
 *  @param[in]  pElement    Pointer to the element to compare.
 *
 *  @return Result of comparison.
 *  @retval < 0   Reference is greater than element.
 *  @retval = 0   Reference is equal to element.
 *  @retval > 0   Reference is less than element.
 *
 */
static int32_t lv_font_codeCompare (const void* pRef,
                                    const void* pElement)
{
    return (*(uint32_t*) pRef) - (*(uint32_t*) pElement);
}
