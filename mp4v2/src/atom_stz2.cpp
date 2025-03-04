/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is MPEG4IP.
 *
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 *
 * Contributor(s):
 *      Dave Mackie     dmackie@cisco.com
 */

#include "src/impl.h"

namespace mp4v2
{
    namespace impl
    {

        ///////////////////////////////////////////////////////////////////////////////

        /*
         * This is used for the 4 bit sample size below.  We need the
         * sampleCount to be correct for the number of samples, but the table
         * size needs to be correct to read and write it.
         */

        class MP4HalfSizeTableProperty : public MP4TableProperty
        {
        public:
            MP4HalfSizeTableProperty(MP4Atom& parentAtom, const char* name,
                                     MP4IntegerProperty* pCountProperty)
                : MP4TableProperty(parentAtom, name, pCountProperty) {};

            // The count is half the actual size
            uint32_t GetCount()
            {
                return (m_pCountProperty->GetValue() + 1) / 2;
            };

            void SetCount(uint32_t count)
            {
                m_pCountProperty->SetValue(count * 2);
            };

        private:
            MP4HalfSizeTableProperty();
            MP4HalfSizeTableProperty(const MP4HalfSizeTableProperty& src);
            MP4HalfSizeTableProperty&
            operator=(const MP4HalfSizeTableProperty& src);
        };

        MP4Stz2Atom::MP4Stz2Atom(MP4File& file)
            : MP4Atom(file, "stz2")
        {
            AddVersionAndFlags(); /* 0, 1 */

            AddReserved(*this, "reserved", 3); /* 2 */

            AddProperty(/* 3 */
                        new MP4Integer8Property(*this, "fieldSize"));

            MP4Integer32Property* pCount =
                new MP4Integer32Property(*this, "sampleCount");
            AddProperty(pCount); /* 4 */
        }

        void MP4Stz2Atom::Read()
        {
            ReadProperties(0, 4);

            uint8_t fieldSize =
                ((MP4Integer8Property*)m_pProperties[3])->GetValue();
            //  uint32_t sampleCount = 0;

            MP4Integer32Property* pCount =
                (MP4Integer32Property*)m_pProperties[4];

            MP4TableProperty* pTable;
            if (fieldSize != 4)
            {
                pTable = new MP4TableProperty(*this, "entries", pCount);
            }
            else
            {
                // 4 bit field size uses a special table.
                pTable = new MP4HalfSizeTableProperty(*this, "entries", pCount);
            }

            AddProperty(pTable);

            if (fieldSize == 16)
            {
                pTable->AddProperty(/* 5/0 */
                                    new MP4Integer16Property(*this,
                                                             "entrySize"));
            }
            else
            {
                pTable->AddProperty(/* 5/0 */
                                    new MP4Integer8Property(*this,
                                                            "entrySize"));
            }

            ReadProperties(4);

            Skip(); // to end of atom
        }

        ///////////////////////////////////////////////////////////////////////////////

    } // namespace impl
} // namespace mp4v2
