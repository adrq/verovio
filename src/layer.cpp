/////////////////////////////////////////////////////////////////////////////
// Name:        layer.cpp
// Author:      Laurent Pugin
// Created:     2011
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "layer.h"

//----------------------------------------------------------------------------

#include <assert.h>

//----------------------------------------------------------------------------

#include "accid.h"
#include "clef.h"
#include "custos.h"
#include "doc.h"
#include "functorparams.h"
#include "keysig.h"
#include "measure.h"
#include "mensur.h"
#include "metersig.h"
#include "note.h"
#include "rpt.h"
#include "staff.h"
#include "vrv.h"

namespace vrv {

//----------------------------------------------------------------------------
// Layer
//----------------------------------------------------------------------------

Layer::Layer() : Object("layer-"), DrawingListInterface(), ObjectListInterface(), AttCommon()
{
    RegisterAttClass(ATT_COMMON);

    // owned pointers need to be set to NULL;
    m_staffDefClef = NULL;
    m_staffDefKeySig = NULL;
    m_staffDefMensur = NULL;
    m_staffDefMeterSig = NULL;

    Reset();
}

Layer::~Layer()
{
    // We need to delete own objects
    Reset();
}

void Layer::Reset()
{
    Object::Reset();
    DrawingListInterface::Reset();
    ResetCommon();

    ResetStaffDefOjects();

    m_drawingStemDir = STEMDIRECTION_NONE;
}

void Layer::ResetStaffDefOjects()
{
    m_drawKeySigCancellation = false;
    // Resert(m_staffDefClef);
    if (m_staffDefClef) {
        delete m_staffDefClef;
        m_staffDefClef = NULL;
    }
    if (m_staffDefKeySig) {
        delete m_staffDefKeySig;
        m_staffDefKeySig = NULL;
    }
    if (m_staffDefMensur) {
        delete m_staffDefMensur;
        m_staffDefMensur = NULL;
    }
    if (m_staffDefMeterSig) {
        delete m_staffDefMeterSig;
        m_staffDefMeterSig = NULL;
    }
}

void Layer::AddLayerElement(LayerElement *element, int idx)
{
    element->SetParent(this);
    if (idx == -1) {
        m_children.push_back(element);
    }
    else {
        InsertChild(element, idx);
    }
    Modify();
}

LayerElement *Layer::GetPrevious(LayerElement *element)
{
    this->ResetList(this);
    if (!element || this->GetList(this)->empty()) return NULL;

    return dynamic_cast<LayerElement *>(GetListPrevious(element));
}

LayerElement *Layer::GetAtPos(int x)
{
    Object *first = this->GetFirst();
    if (!first || !first->IsLayerElement()) return NULL;

    LayerElement *element = dynamic_cast<LayerElement *>(first);
    assert(element);
    if (element->GetDrawingX() > x) return NULL;

    Object *next;
    while ((next = this->GetNext())) {
        if (!next->IsLayerElement()) continue;
        LayerElement *nextLayerElement = dynamic_cast<LayerElement *>(next);
        assert(nextLayerElement);
        if (nextLayerElement->GetDrawingX() > x) return element;
        element = nextLayerElement;
    }
    return element;
}

Clef *Layer::GetClef(LayerElement *test)
{
    Object *testObject = test;

    if (!test) {
        return GetCurrentClef();
    }

    // make sure list is set
    ResetList(this);
    if (test->Is() != CLEF) {
        testObject = GetListFirstBackward(testObject, CLEF);
    }

    if (testObject && testObject->Is() == CLEF) {
        Clef *clef = dynamic_cast<Clef *>(testObject);
        assert(clef);
        return clef;
    }

    return GetCurrentClef();
}

int Layer::GetClefOffset(LayerElement *test)
{
    Clef *clef = GetClef(test);
    if (!clef) return 0;
    return clef->GetClefOffset();
}

Clef *Layer::GetCurrentClef() const
{
    Staff *staff = dynamic_cast<Staff *>(this->GetFirstParent(STAFF));
    assert(staff && staff->m_drawingStaffDef && staff->m_drawingStaffDef->GetCurrentClef());
    return staff->m_drawingStaffDef->GetCurrentClef();
}

KeySig *Layer::GetCurrentKeySig() const
{
    Staff *staff = dynamic_cast<Staff *>(this->GetFirstParent(STAFF));
    assert(staff && staff->m_drawingStaffDef);
    return staff->m_drawingStaffDef->GetCurrentKeySig();
}

Mensur *Layer::GetCurrentMensur() const
{
    Staff *staff = dynamic_cast<Staff *>(this->GetFirstParent(STAFF));
    assert(staff && staff->m_drawingStaffDef);
    return staff->m_drawingStaffDef->GetCurrentMensur();
}

MeterSig *Layer::GetCurrentMeterSig() const
{
    Staff *staff = dynamic_cast<Staff *>(this->GetFirstParent(STAFF));
    assert(staff && staff->m_drawingStaffDef);
    return staff->m_drawingStaffDef->GetCurrentMeterSig();
}

void Layer::SetDrawingAndCurrentValues(StaffDef *currentStaffDef)
{
    if (!currentStaffDef) {
        LogDebug("staffDef not found");
        return;
    }

    // Remove any previous value in the Layer
    this->ResetStaffDefOjects();

    // Special case with C-major / A-minor key signature (0) : if key cancellation is false, we are at the beginning
    // of a new system, and hence we should not draw it. Maybe this can be improved?
    bool drawKeySig = currentStaffDef->DrawKeySig();
    if (currentStaffDef->GetCurrentKeySig() && (currentStaffDef->GetCurrentKeySig()->GetAlterationNumber() == 0)) {
        if (currentStaffDef->DrawKeySigCancellation() == false) {
            drawKeySig = false;
        }
    }

    if (currentStaffDef->DrawClef()) this->m_staffDefClef = new Clef(*currentStaffDef->GetCurrentClef());
    // special case - see above
    if (drawKeySig) this->m_staffDefKeySig = new KeySig(*currentStaffDef->GetCurrentKeySig());
    if (currentStaffDef->DrawMensur()) this->m_staffDefMensur = new Mensur(*currentStaffDef->GetCurrentMensur());
    if (currentStaffDef->DrawMeterSig())
        this->m_staffDefMeterSig = new MeterSig(*currentStaffDef->GetCurrentMeterSig());
    this->SetDrawKeySigCancellation(currentStaffDef->DrawKeySigCancellation());

    // Don't draw on the next one
    currentStaffDef->SetDrawClef(false);
    currentStaffDef->SetDrawKeySig(false);
    currentStaffDef->SetDrawMensur(false);
    currentStaffDef->SetDrawMeterSig(false);
    currentStaffDef->SetDrawKeySigCancellation(false);
}

//----------------------------------------------------------------------------
// Layer functor methods
//----------------------------------------------------------------------------

int Layer::UnsetCurrentScoreDef(FunctorParams *functorParams)
{
    ResetStaffDefOjects();

    return FUNCTOR_CONTINUE;
};

int Layer::AlignHorizontally(FunctorParams *functorParams)
{
    AlignHorizontallyParams *params = dynamic_cast<AlignHorizontallyParams *>(functorParams);
    assert(params);

    params->m_currentMensur = GetCurrentMensur();
    params->m_currentMeterSig = GetCurrentMeterSig();

    // We are starting a new layer, reset the time;
    // We set it to -1.0 for the scoreDef attributes since they have to be aligned before any timestamp event (-1.0)
    params->m_time = DUR_MAX * -1.0;

    if (this->GetStaffDefClef()) {
        GetStaffDefClef()->AlignHorizontally(params);
    }
    if (this->GetStaffDefKeySig()) {
        GetStaffDefKeySig()->AlignHorizontally(params);
    }
    if (this->GetStaffDefMensur()) {
        GetStaffDefMensur()->AlignHorizontally(params);
    }
    if (GetStaffDefMeterSig()) {
        GetStaffDefMeterSig()->AlignHorizontally(params);
    }

    // Now we have to set it to 0.0 since we will start aligning muscial content
    params->m_time = 0.0;

    return FUNCTOR_CONTINUE;
}

int Layer::AlignHorizontallyEnd(FunctorParams *functorParams)
{
    AlignHorizontallyParams *params = dynamic_cast<AlignHorizontallyParams *>(functorParams);
    assert(params);

    int i;
    for (i = 0; i < params->m_measureAligner->GetChildCount(); i++) {
        Alignment *alignment = dynamic_cast<Alignment *>(params->m_measureAligner->GetChild(i));
        assert(alignment);
        if (alignment->HasGraceAligner()) {
            alignment->GetGraceAligner()->AlignStack();
        }
    }

    return FUNCTOR_CONTINUE;
}

int Layer::PrepareProcessingLists(FunctorParams *functorParams)
{
    PrepareProcessingListsParams *params = dynamic_cast<PrepareProcessingListsParams *>(functorParams);
    assert(params);

    // Alternate solution with StaffN_LayerN_VerseN_t
    // StaffN_LayerN_VerseN_t *tree = static_cast<StaffN_LayerN_VerseN_t*>((*params).at(0));

    Staff *staff = dynamic_cast<Staff *>(this->GetFirstParent(STAFF));
    assert(staff);
    params->m_layerTree.child[staff->GetN()].child[this->GetN()];

    return FUNCTOR_CONTINUE;
}

int Layer::SetDrawingXY(FunctorParams *functorParams)
{
    SetDrawingXYParams *params = dynamic_cast<SetDrawingXYParams *>(functorParams);
    assert(params);

    params->m_currentLayer = this;

    // Second pass where we do just process layer elements
    if (params->m_processLayerElements) {
        return FUNCTOR_CONTINUE;
    }

    // set the values for the scoreDef elements when required
    if (this->GetStaffDefClef()) {
        this->GetStaffDefClef()->SetDrawingX(
            this->GetStaffDefClef()->GetXRel() + params->m_currentMeasure->GetDrawingX());
    }
    if (this->GetStaffDefKeySig()) {
        this->GetStaffDefKeySig()->SetDrawingX(
            this->GetStaffDefKeySig()->GetXRel() + params->m_currentMeasure->GetDrawingX());
    }
    if (this->GetStaffDefMensur()) {
        this->GetStaffDefMensur()->SetDrawingX(
            this->GetStaffDefMensur()->GetXRel() + params->m_currentMeasure->GetDrawingX());
    }
    if (this->GetStaffDefMeterSig()) {
        this->GetStaffDefMeterSig()->SetDrawingX(
            this->GetStaffDefMeterSig()->GetXRel() + params->m_currentMeasure->GetDrawingX());
    }

    return FUNCTOR_CONTINUE;
}

int Layer::PrepareRpt(FunctorParams *functorParams)
{
    PrepareRptParams *params = dynamic_cast<PrepareRptParams *>(functorParams);
    assert(params);

    // If we have encountered a mRpt before and there is none is this layer, reset it to NULL
    if (params->m_currentMRpt && !this->FindChildByType(MRPT)) {
        params->m_currentMRpt = NULL;
    }
    return FUNCTOR_CONTINUE;
}

int Layer::CalcMaxMeasureDuration(FunctorParams *functorParams)
{
    CalcMaxMeasureDurationParams *params = dynamic_cast<CalcMaxMeasureDurationParams *>(functorParams);
    assert(params);

    // reset it
    params->m_currentValue = 0.0;

    return FUNCTOR_CONTINUE;
}

} // namespace vrv