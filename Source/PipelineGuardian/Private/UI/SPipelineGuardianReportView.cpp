// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/SPipelineGuardianReportView.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Analysis/FAssetAnalysisResult.h"
#include "SlateOptMacros.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetRegistry/AssetData.h"
#include "PipelineGuardian.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "SPipelineGuardianReportView"

// Helper function declarations
FText SeverityToText(EAssetIssueSeverity Severity);
FSlateColor GetSeverityColor(EAssetIssueSeverity Severity);

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SPipelineGuardianReportView::Construct(const FArguments& InArgs)
{
    DisplayedResults.Empty();
    AllResults.Empty();
    SelectionState.Empty();

    // Initialize severity filter options
    SeverityFilterOptions.Empty();
    SeverityFilterOptions.Add(MakeShared<FString>(TEXT("All")));
    SeverityFilterOptions.Add(MakeShared<FString>(TEXT("Critical")));
    SeverityFilterOptions.Add(MakeShared<FString>(TEXT("Error")));
    SeverityFilterOptions.Add(MakeShared<FString>(TEXT("Warning")));
    SeverityFilterOptions.Add(MakeShared<FString>(TEXT("Info")));
    CurrentSeverityFilter = SeverityFilterOptions[0]; // Default to "All"

    ChildSlot
    [
        SNew(SVerticalBox)
        // Filter and Selection Controls
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5.f)
        [
            SNew(SHorizontalBox)
            // Select All Checkbox
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(5.f, 0.f)
            [
                SAssignNew(SelectAllCheckBox, SCheckBox)
                .OnCheckStateChanged(this, &SPipelineGuardianReportView::OnSelectAllChanged)
                .IsChecked(this, &SPipelineGuardianReportView::GetSelectAllState)
                .ToolTipText(LOCTEXT("SelectAllTooltip", "Select/Deselect all visible items"))
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("SelectAllLabel", "Select All"))
                ]
            ]
            // Severity Filter
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(15.f, 0.f, 5.f, 0.f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("SeverityFilterLabel", "Filter by Severity:"))
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(5.f, 0.f)
            [
                SAssignNew(SeverityFilterComboBox, SComboBox<TSharedPtr<FString>>)
                .OptionsSource(&SeverityFilterOptions)
                .OnGenerateWidget(this, &SPipelineGuardianReportView::GenerateSeverityFilterWidget)
                .OnSelectionChanged(this, &SPipelineGuardianReportView::OnSeverityFilterChanged)
                .InitiallySelectedItem(CurrentSeverityFilter)
                [
                    SNew(STextBlock)
                    .Text(this, &SPipelineGuardianReportView::GetSeverityFilterText)
                ]
            ]
            // Spacer
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SNew(SSpacer)
            ]
            // Fix Selected Button
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(5.f, 0.f)
            [
                SNew(SButton)
                .Text(this, &SPipelineGuardianReportView::GetFixSelectedButtonText)
                .ToolTipText(LOCTEXT("FixSelectedTooltip", "Fix only the selected issues"))
                .OnClicked(this, &SPipelineGuardianReportView::OnFixSelectedClicked)
                .IsEnabled(this, &SPipelineGuardianReportView::IsFixSelectedEnabled)
                .ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
            ]
            // Fix All Button
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(5.f, 0.f)
            [
                SNew(SButton)
                .Text(LOCTEXT("FixAllButton", "Fix All"))
                .ToolTipText(LOCTEXT("FixAllTooltip", "Fix all issues that have available fixes"))
                .OnClicked(this, &SPipelineGuardianReportView::OnFixAllClicked)
                .IsEnabled(this, &SPipelineGuardianReportView::IsFixAllEnabled)
                .ButtonStyle(FAppStyle::Get(), "PrimaryButton")
            ]
        ]
        // Results List
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        [
            SAssignNew(ResultsListView, SListView<TSharedPtr<FAssetAnalysisResult>>)
            .ListItemsSource(&DisplayedResults)
            .OnGenerateRow(this, &SPipelineGuardianReportView::OnGenerateRowForResultList)
            .OnMouseButtonDoubleClick(this, &SPipelineGuardianReportView::OnResultDoubleClick)
            .HeaderRow
            (
                SNew(SHeaderRow)
                + SHeaderRow::Column("Selection").DefaultLabel(LOCTEXT("SelectionHeader", "Select")).FixedWidth(60.f)
                + SHeaderRow::Column("AssetName").DefaultLabel(LOCTEXT("AssetNameHeader", "Asset Name")).FillWidth(0.25f)
                + SHeaderRow::Column("IssueDescription").DefaultLabel(LOCTEXT("IssueDescriptionHeader", "Description")).FillWidth(0.5f)
                + SHeaderRow::Column("Severity").DefaultLabel(LOCTEXT("SeverityHeader", "Severity")).FillWidth(0.15f)
                + SHeaderRow::Column("Actions").DefaultLabel(LOCTEXT("ActionsHeader", "Actions")).FixedWidth(80.f)
            )
        ]
    ];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SPipelineGuardianReportView::SetResults(const TArray<TSharedPtr<FAssetAnalysisResult>>& InResults)
{
    AllResults = InResults;
    SelectionState.Empty();
    
    // Initialize selection state for all results
    for (const TSharedPtr<FAssetAnalysisResult>& Result : AllResults)
    {
        SelectionState.Add(Result, false);
    }
    
    ApplyFilters();
}

FText SPipelineGuardianReportView::GetFixSelectedButtonText() const
{
    int32 SelectedCount = GetSelectedItemCount();
    if (SelectedCount == 0)
    {
        return LOCTEXT("FixSelectedButton", "Fix Selected");
    }
    return FText::Format(LOCTEXT("FixSelectedButtonWithCount", "Fix Selected ({0})"), FText::AsNumber(SelectedCount));
}

FReply SPipelineGuardianReportView::OnFixSelectedClicked()
{
    // Get selected fixable issues
    TArray<TSharedPtr<FAssetAnalysisResult>> SelectedFixableResults;
    for (const TSharedPtr<FAssetAnalysisResult>& Result : DisplayedResults)
    {
        if (Result.IsValid() && SelectionState.Contains(Result) && SelectionState[Result] && Result->FixAction.IsBound())
        {
            SelectedFixableResults.Add(Result);
        }
    }
    
    if (SelectedFixableResults.Num() == 0)
    {
        FMessageDialog::Open(EAppMsgType::Ok, 
            LOCTEXT("NoSelectedFixableMessage", "No selected items have available fixes."),
            LOCTEXT("NoSelectedFixableTitle", "No Fixes Available"));
        return FReply::Handled();
    }
    
    ExecuteFixesAndRefresh(SelectedFixableResults);
    return FReply::Handled();
}

FReply SPipelineGuardianReportView::OnFixAllClicked()
{
    // Get all fixable issues from displayed results
    TArray<TSharedPtr<FAssetAnalysisResult>> FixableResults;
    for (const TSharedPtr<FAssetAnalysisResult>& Result : DisplayedResults)
    {
        if (Result.IsValid() && Result->FixAction.IsBound())
        {
            FixableResults.Add(Result);
        }
    }
    
    if (FixableResults.Num() == 0)
    {
        FMessageDialog::Open(EAppMsgType::Ok, 
            LOCTEXT("NoFixableMessage", "No items have available fixes."),
            LOCTEXT("NoFixableTitle", "No Fixes Available"));
        return FReply::Handled();
    }
    
    ExecuteFixesAndRefresh(FixableResults);
    return FReply::Handled();
}

void SPipelineGuardianReportView::ExecuteFixesAndRefresh(const TArray<TSharedPtr<FAssetAnalysisResult>>& ItemsToFix)
{
    // Show confirmation dialog
    FText ConfirmationMessage = FText::Format(
        LOCTEXT("FixConfirmationMessage", "This will automatically fix {0} issue(s). This operation cannot be undone. Continue?"),
        FText::AsNumber(ItemsToFix.Num())
    );
    
    EAppReturnType::Type UserResponse = FMessageDialog::Open(
        EAppMsgType::YesNo,
        ConfirmationMessage,
        LOCTEXT("FixConfirmationTitle", "Confirm Asset Fixes")
    );
    
    if (UserResponse == EAppReturnType::Yes)
    {
        // User confirmed, execute all fix actions
        int32 SuccessCount = 0;
        for (const TSharedPtr<FAssetAnalysisResult>& Result : ItemsToFix)
        {
            if (Result.IsValid() && Result->FixAction.IsBound())
            {
                Result->FixAction.ExecuteIfBound();
                SuccessCount++;
            }
        }
        
        UE_LOG(LogPipelineGuardian, Log, TEXT("Applied fixes to %d assets"), SuccessCount);
        
        // Show completion message
        FText CompletionMessage = FText::Format(
            LOCTEXT("FixCompletionMessage", "Successfully applied fixes to {0} asset(s).\n\nRefreshing analysis results..."),
            FText::AsNumber(SuccessCount)
        );
        FMessageDialog::Open(EAppMsgType::Ok, CompletionMessage, LOCTEXT("FixCompletionTitle", "Fixes Applied"));
        
        // Request refresh from parent window
        if (OnRefreshRequested.IsBound())
        {
            OnRefreshRequested.Execute();
        }
    }
}

bool SPipelineGuardianReportView::IsFixSelectedEnabled() const
{
    for (const TSharedPtr<FAssetAnalysisResult>& Result : DisplayedResults)
    {
        if (Result.IsValid() && SelectionState.Contains(Result) && SelectionState[Result] && Result->FixAction.IsBound())
        {
            return true;
        }
    }
    return false;
}

bool SPipelineGuardianReportView::IsFixAllEnabled() const
{
    for (const TSharedPtr<FAssetAnalysisResult>& Result : DisplayedResults)
    {
        if (Result.IsValid() && Result->FixAction.IsBound())
        {
            return true;
        }
    }
    return false;
}

void SPipelineGuardianReportView::OnItemSelectionChanged(ECheckBoxState NewState, TSharedPtr<FAssetAnalysisResult> Item)
{
    if (Item.IsValid() && SelectionState.Contains(Item))
    {
        SelectionState[Item] = (NewState == ECheckBoxState::Checked);
    }
}

void SPipelineGuardianReportView::OnSelectAllChanged(ECheckBoxState NewState)
{
    bool bNewState = (NewState == ECheckBoxState::Checked);
    for (const TSharedPtr<FAssetAnalysisResult>& Result : DisplayedResults)
    {
        if (Result.IsValid() && SelectionState.Contains(Result))
        {
            SelectionState[Result] = bNewState;
        }
    }
    
    // Refresh the list to update checkboxes
    if (ResultsListView.IsValid())
    {
        ResultsListView->RequestListRefresh();
    }
}

ECheckBoxState SPipelineGuardianReportView::GetSelectAllState() const
{
    if (DisplayedResults.Num() == 0)
    {
        return ECheckBoxState::Unchecked;
    }
    
    int32 SelectedCount = 0;
    for (const TSharedPtr<FAssetAnalysisResult>& Result : DisplayedResults)
    {
        if (Result.IsValid() && SelectionState.Contains(Result) && SelectionState[Result])
        {
            SelectedCount++;
        }
    }
    
    if (SelectedCount == 0)
    {
        return ECheckBoxState::Unchecked;
    }
    else if (SelectedCount == DisplayedResults.Num())
    {
        return ECheckBoxState::Checked;
    }
    else
    {
        return ECheckBoxState::Undetermined;
    }
}

void SPipelineGuardianReportView::OnSeverityFilterChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
    CurrentSeverityFilter = NewSelection;
    ApplyFilters();
}

FText SPipelineGuardianReportView::GetSeverityFilterText() const
{
    return CurrentSeverityFilter.IsValid() ? FText::FromString(*CurrentSeverityFilter) : LOCTEXT("AllSeverities", "All");
}

TSharedRef<SWidget> SPipelineGuardianReportView::GenerateSeverityFilterWidget(TSharedPtr<FString> InOption)
{
    return SNew(STextBlock)
        .Text(FText::FromString(*InOption));
}

void SPipelineGuardianReportView::ApplyFilters()
{
    DisplayedResults.Empty();
    
    for (const TSharedPtr<FAssetAnalysisResult>& Result : AllResults)
    {
        if (!Result.IsValid())
        {
            continue;
        }
        
        // Apply severity filter
        bool bPassesSeverityFilter = true;
        if (CurrentSeverityFilter.IsValid() && *CurrentSeverityFilter != TEXT("All"))
        {
            FString SeverityString = SeverityToText(Result->Severity).ToString();
            bPassesSeverityFilter = (SeverityString == *CurrentSeverityFilter);
        }
        
        if (bPassesSeverityFilter)
        {
            DisplayedResults.Add(Result);
        }
    }
    
    // Refresh the list view
    if (ResultsListView.IsValid())
    {
        ResultsListView->RequestListRefresh();
    }
}

int32 SPipelineGuardianReportView::GetSelectedItemCount() const
{
    int32 Count = 0;
    for (const TSharedPtr<FAssetAnalysisResult>& Result : DisplayedResults)
    {
        if (Result.IsValid() && SelectionState.Contains(Result) && SelectionState[Result])
        {
            Count++;
        }
    }
    return Count;
}

void SPipelineGuardianReportView::OnResultDoubleClick(TSharedPtr<FAssetAnalysisResult> InItem)
{
    if (InItem.IsValid() && InItem->Asset.IsValid())
    {
        TArray<FAssetData> AssetsToSync;
        AssetsToSync.Add(InItem->Asset);

        FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
        ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToSync);
    }
}

// Helper to convert EAssetIssueSeverity to FText
FText SeverityToText(EAssetIssueSeverity Severity)
{
    switch (Severity)
    {
        case EAssetIssueSeverity::Critical: return LOCTEXT("SeverityCritical", "Critical");
        case EAssetIssueSeverity::Error: return LOCTEXT("SeverityError", "Error");
        case EAssetIssueSeverity::Warning: return LOCTEXT("SeverityWarning", "Warning");
        case EAssetIssueSeverity::Info: return LOCTEXT("SeverityInfo", "Info");
        default: return LOCTEXT("SeverityUnknown", "Unknown");
    }
}

// Helper to get color based on severity
FSlateColor GetSeverityColor(EAssetIssueSeverity Severity)
{
    switch (Severity)
    {
        case EAssetIssueSeverity::Critical: return FSlateColor(FLinearColor::Red);
        case EAssetIssueSeverity::Error: return FSlateColor(FLinearColor(1.0f, 0.3f, 0.3f)); // Light Red
        case EAssetIssueSeverity::Warning: return FSlateColor(FLinearColor(1.0f, 0.8f, 0.0f)); // Orange/Yellow
        case EAssetIssueSeverity::Info: return FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)); // Light Gray
        default: return FSlateColor::UseForeground();
    }
}

TSharedRef<ITableRow> SPipelineGuardianReportView::OnGenerateRowForResultList(TSharedPtr<FAssetAnalysisResult> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
    FSlateColor RowColor = GetSeverityColor(Item->Severity);
    
    return SNew(STableRow<TSharedPtr<FAssetAnalysisResult>>, OwnerTable)
    .Padding(FMargin(0, 2, 0, 2))
    [
        SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .BorderBackgroundColor(RowColor.GetSpecifiedColor() * 0.1f) // Subtle background tint
        [
            SNew(SHorizontalBox)
            // Selection Checkbox
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(10.f, 0.f)
            .VAlign(VAlign_Center)
            .HAlign(HAlign_Center)
            [
                SNew(SCheckBox)
                .IsChecked_Lambda([this, Item]() -> ECheckBoxState
                {
                    if (SelectionState.Contains(Item) && SelectionState[Item])
                    {
                        return ECheckBoxState::Checked;
                    }
                    return ECheckBoxState::Unchecked;
                })
                .OnCheckStateChanged(this, &SPipelineGuardianReportView::OnItemSelectionChanged, Item)
            ]
            // Asset Name
            + SHorizontalBox::Slot()
            .FillWidth(0.25f)
            .Padding(10.f, 0.f)
            .VAlign(VAlign_Center)
            .HAlign(HAlign_Left)
            [
                SNew(STextBlock)
                .Text(FText::FromName(Item->Asset.AssetName))
                .ColorAndOpacity(RowColor)
            ]
            // Description
            + SHorizontalBox::Slot()
            .FillWidth(0.5f)
            .Padding(10.f, 0.f)
            .VAlign(VAlign_Center)
            .HAlign(HAlign_Left)
            [
                SNew(STextBlock)
                .Text(Item->Description)
                .ColorAndOpacity(RowColor)
                .AutoWrapText(true)
            ]
            // Severity
            + SHorizontalBox::Slot()
            .FillWidth(0.15f)
            .Padding(10.f, 0.f)
            .VAlign(VAlign_Center)
            .HAlign(HAlign_Left)
            [
                SNew(STextBlock)
                .Text(SeverityToText(Item->Severity))
                .ColorAndOpacity(RowColor)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
            ]
            // Individual Fix Button
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(5.f, 0.f)
            .VAlign(VAlign_Center)
            .HAlign(HAlign_Center)
            [
                SNew(SButton)
                .Text(LOCTEXT("FixButton", "Fix"))
                .ToolTipText(LOCTEXT("FixButtonTooltip", "Fix this specific issue"))
                .OnClicked_Lambda([this, Item]() -> FReply
                {
                    if (Item.IsValid() && Item->FixAction.IsBound())
                    {
                        TArray<TSharedPtr<FAssetAnalysisResult>> SingleItem;
                        SingleItem.Add(Item);
                        ExecuteFixesAndRefresh(SingleItem);
                    }
                    return FReply::Handled();
                })
                .IsEnabled_Lambda([Item]() -> bool
                {
                    return Item.IsValid() && Item->FixAction.IsBound();
                })
                .ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
            ]
        ]
    ];
}

#undef LOCTEXT_NAMESPACE 