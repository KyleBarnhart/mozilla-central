/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ChangeAttributeTxn.h"
#include "nsAString.h"
#include "nsDebug.h"                    // for NS_ASSERTION
#include "nsError.h"                    // for NS_ERROR_NOT_INITIALIZED, etc
#include "nsEditor.h"                   // for nsEditor
#include "nsString.h"                   // for nsString
#include "mozilla/dom/Element.h"

using namespace mozilla;

ChangeAttributeTxn::ChangeAttributeTxn()
  : EditTxn()
{
}

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(ChangeAttributeTxn, EditTxn)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mElement)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(ChangeAttributeTxn, EditTxn)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(ChangeAttributeTxn, EditTxn)
NS_IMPL_RELEASE_INHERITED(ChangeAttributeTxn, EditTxn)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(ChangeAttributeTxn)
NS_INTERFACE_MAP_END_INHERITING(EditTxn)

NS_IMETHODIMP ChangeAttributeTxn::Init(nsEditor      *aEditor,
                                       dom::Element *aElement,
                                       const nsAString& aAttribute,
                                       const nsAString& aValue,
                                       bool aRemoveAttribute)
{
  NS_ASSERTION(aEditor && aElement, "bad arg");
  if (!aEditor || !aElement) { return NS_ERROR_NULL_POINTER; }

  mEditor = aEditor;
  mElement = aElement;
  mAttribute = aAttribute;
  mValue = aValue;
  mRemoveAttribute = aRemoveAttribute;
  mAttributeWasSet=false;
  mUndoValue.Truncate();
  return NS_OK;
}

NS_IMETHODIMP ChangeAttributeTxn::DoTransaction(void)
{
  NS_ASSERTION(mEditor && mElement, "bad state");
  if (!mEditor || !mElement) { return NS_ERROR_NOT_INITIALIZED; }

  nsCOMPtr<nsIDOMElement> element = do_QueryInterface(mElement);
  // need to get the current value of the attribute and save it, and set mAttributeWasSet
  nsresult result = mEditor->GetAttributeValue(element, mAttribute, mUndoValue, &mAttributeWasSet);
  NS_ENSURE_SUCCESS(result, result);

  // XXX: hack until attribute-was-set code is implemented
  if (!mUndoValue.IsEmpty())
    mAttributeWasSet = true;
  // XXX: end hack

  ErrorResult rv;
  // now set the attribute to the new value
  if (!mRemoveAttribute)
    mElement->SetAttribute(mAttribute, mValue, rv);
  else
    mElement->RemoveAttribute(mAttribute, rv);

  return rv.ErrorCode();
}

NS_IMETHODIMP ChangeAttributeTxn::UndoTransaction(void)
{
  NS_ASSERTION(mEditor && mElement, "bad state");
  if (!mEditor || !mElement) { return NS_ERROR_NOT_INITIALIZED; }

  ErrorResult rv;
  if (mAttributeWasSet)
    mElement->SetAttribute(mAttribute, mUndoValue, rv);
  else
    mElement->RemoveAttribute(mAttribute, rv);

  return rv.ErrorCode();
}

NS_IMETHODIMP ChangeAttributeTxn::RedoTransaction(void)
{
  NS_ASSERTION(mEditor && mElement, "bad state");
  if (!mEditor || !mElement) { return NS_ERROR_NOT_INITIALIZED; }

  ErrorResult rv;
  if (!mRemoveAttribute)
    mElement->SetAttribute(mAttribute, mValue, rv);
  else
    mElement->RemoveAttribute(mAttribute, rv);

  return rv.ErrorCode();
}

NS_IMETHODIMP ChangeAttributeTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("ChangeAttributeTxn: [mRemoveAttribute == ");

  if (!mRemoveAttribute)
    aString.AppendLiteral("false] ");
  else
    aString.AppendLiteral("true] ");
  aString += mAttribute;
  return NS_OK;
}
