// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Analysis/FAssetAnalysisResult.h" // For FAssetAnalysisResult
#include "Widgets/Views/SListView.h" // For SListView

// Forward declarations
class SCheckBox;
template<typename OptionType> class SComboBox;

/**
 * Widget for displaying analysis results with filtering and fix capabilities
 */
class SPipelineGuardianReportView : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SPipelineGuardianReportView) {}
    SLATE_END_ARGS()

    /** Constructs this widget with InArgs */
    void Construct(const FArguments& InArgs);

    /**
     * Sets the list of analysis results to display.
     * @param InResults The array of shared pointers to analysis results.
     */
    void SetResults(const TArray<TSharedPtr<FAssetAnalysisResult>>& InResults);

    /**
     * Refreshes the results by re-analyzing the same assets.
     * This is called after fixes are applied to update the list.
     */
    DECLARE_DELEGATE(FOnRefreshRequested);
    FOnRefreshRequested OnRefreshRequested;

private:
    /** The list view widget for displaying results. */
    TSharedPtr<SListView<TSharedPtr<FAssetAnalysisResult>>> ResultsListView;

    /** The source array for the list view. */
    TArray<TSharedPtr<FAssetAnalysisResult>> DisplayedResults;

    /** All results (before filtering) */
    TArray<TSharedPtr<FAssetAnalysisResult>> AllResults;

    /** Selection state for each result */
    TMap<TSharedPtr<FAssetAnalysisResult>, bool> SelectionState;

    /** Severity filter options */
    TArray<TSharedPtr<FString>> SeverityFilterOptions;
    TSharedPtr<SComboBox<TSharedPtr<FString>>> SeverityFilterComboBox;
    TSharedPtr<FString> CurrentSeverityFilter;

    /** Select All checkbox */
    TSharedPtr<SCheckBox> SelectAllCheckBox;

    /**
     * Generates a row widget for the ResultsListView.
     * @param Item The FAssetAnalysisResult item for this row.
     * @param OwnerTable The SListView that owns this row.
     * @return A new SWidget for the row.
     */
    TSharedRef<ITableRow> OnGenerateRowForResultList(TSharedPtr<FAssetAnalysisResult> Item, const TSharedRef<STableViewBase>& OwnerTable);

    /**
     * Handler for double click on result rows. Syncs the Content Browser to the asset.
     * @param InItem The FAssetAnalysisResult item that was double-clicked.
     */
    void OnResultDoubleClick(TSharedPtr<FAssetAnalysisResult> InItem);

    /** Handle Fix Now button click (for selected items only) */
    FReply OnFixSelectedClicked();
    
    /** Handle Fix All button click */
    FReply OnFixAllClicked();
    
    /** Check if Fix Selected button should be enabled */
    bool IsFixSelectedEnabled() const;
    
    /** Check if Fix All button should be enabled */
    bool IsFixAllEnabled() const;

    /** Get text for Fix Selected button with count */
    FText GetFixSelectedButtonText() const;

    /** Handle selection checkbox change for individual items */
    void OnItemSelectionChanged(ECheckBoxState NewState, TSharedPtr<FAssetAnalysisResult> Item);

    /** Handle Select All checkbox change */
    void OnSelectAllChanged(ECheckBoxState NewState);

    /** Get the state of Select All checkbox */
    ECheckBoxState GetSelectAllState() const;

    /** Handle severity filter change */
    void OnSeverityFilterChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

    /** Get text for severity filter combo box */
    FText GetSeverityFilterText() const;

    /** Generate widget for severity filter combo box options */
    TSharedRef<SWidget> GenerateSeverityFilterWidget(TSharedPtr<FString> InOption);

    /** Apply current filters to the results */
    void ApplyFilters();

    /** Get the number of selected items */
    int32 GetSelectedItemCount() const;

    /** Execute fixes and refresh the list */
    void ExecuteFixesAndRefresh(const TArray<TSharedPtr<FAssetAnalysisResult>>& ItemsToFix);
}; 